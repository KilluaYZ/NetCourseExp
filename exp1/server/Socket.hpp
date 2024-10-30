#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <arpa/inet.h>
#include <cmath>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <vector>
using namespace std;

namespace Socket {
const int FRAME_BUF_SIZE = 1016;
const int FRAME_HEAD_SIZE = 8;
const int FRAME_SIZE = 1024;
const int FRAME_TYPE_START = 1;
const int FRAME_TYPE_END = 2;
const int FRAME_TYPE_DATA = 3;
const int FRAME_TYPE_ACK = 4;
const int FRAME_TYPE_REQUEST_DATA = 5;
const int FRAME_TYPE_SEND_DATA = 6;
const int FRAME_TYPE_MSG = 7;

typedef int Socket;
typedef string Ip;
typedef unsigned int Port;

struct _frame {
  uint32_t _id;
  uint16_t _type;
  uint16_t _length;
  char buf[FRAME_BUF_SIZE];
};

class Data {
public:
  int size;
  _frame *buf;
  Data() {
    this->size = 0;
    this->buf = (_frame *)malloc(sizeof(char) * FRAME_SIZE);
    memset(this->buf, 0, sizeof(char) * FRAME_SIZE);
  }

  ~Data() { free(this->buf); }

  void copy(Data *data) {
    if (data == nullptr)
      throw std::runtime_error("Error while copying Data");
    this->size = data->size;
    memcpy(this->buf, data->buf, sizeof(_frame));
  }

  void from_bytes(char *buffer, int size) {
    if (buffer == nullptr)
      throw std::runtime_error("Data::from_bytes buffer is null");
    this->size = size;
    memcpy(this->buf, buffer, size);
  }
};

typedef struct {
  Ip ip;
  Port port;
} Address;

struct sockaddr_in *to_sockaddr(Address *a) {
  struct sockaddr_in *ret;

  ret = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
  ret->sin_family = AF_INET;
  inet_aton(a->ip.c_str(), &(ret->sin_addr));
  ret->sin_port = htons(a->port);

  return ret;
}

Address *from_sockaddr(struct sockaddr_in *address) {
  Address *ret;

  ret = (Address *)malloc(sizeof(Address));
  ret->ip = inet_ntoa(address->sin_addr);
  ret->port = ntohs(address->sin_port);

  return ret;
}

class BinaryStream {
protected:
  std::string file_path;
  uint32_t _size;
  uint32_t _ext_size;
  uint32_t _byte_size;
  uint32_t _cur;
  virtual Data *first() { return nullptr; };
  virtual Data *end() { return nullptr; };
  virtual Data *mid() { return nullptr; };
  virtual void first(Data *data) {};
  virtual void end(Data *data) {};
  virtual void mid(Data *data) {};

public:
  BinaryStream(std::string file_path) {
    this->file_path = file_path;
    this->_size = 0;
    this->_cur = 0;
    this->_ext_size = 0;
    this->_byte_size = 0;
  };
  virtual Data *next() { return nullptr; };
  virtual void next(Data *data) {};
  virtual bool has_next() {
    if (this->_ext_size == 0)
      return true;
    if (this->_cur < this->_ext_size)
      return true;
    return false;
  }
  virtual int size() { return this->_size; }
  virtual int ext_size() { return this->_ext_size; }
};

class iBinaryStream : public BinaryStream {
protected:
  std::ifstream infile;
  Data ret;
  Data *first() override {
    _frame first_frame;
    first_frame._id = 0;
    first_frame._type = FRAME_TYPE_START;
    first_frame._length = size();
    ret.size = FRAME_HEAD_SIZE;
    memcpy((void *)ret.buf, (void *)&first_frame, ret.size);
    return &ret;
  }

  Data *end() override {
    _frame end_frame;
    end_frame._id = ext_size() - 1;
    end_frame._type = FRAME_TYPE_END;
    end_frame._length = 0;
    ret.size = FRAME_HEAD_SIZE;
    memcpy((void *)ret.buf, (void *)&end_frame, ret.size);
    return &ret;
  }

  Data *mid() override {
    _frame mid_frame;
    mid_frame._id = _cur;
    mid_frame._type = FRAME_TYPE_DATA;
    // 读入数据
    infile.read(mid_frame.buf, FRAME_BUF_SIZE);
    // 确定读入的长度
    mid_frame._length = infile.gcount();
    ret.size = FRAME_HEAD_SIZE + mid_frame._length;
    memcpy(ret.buf, (void *)&mid_frame, ret.size);
    return &ret;
  }

public:
  iBinaryStream(std::string file_path) : BinaryStream(file_path) {
    infile = std::ifstream(file_path, ios::binary);
    if (!infile) {
      throw std::runtime_error("Failed to open file.");
    }
    // 获取文件大小
    infile.seekg(0, infile.end);
    this->_byte_size = infile.tellg();
    infile.seekg(0, infile.beg);
    // 获取数据帧数
    this->_size = (int)ceil((double)this->_byte_size / FRAME_BUF_SIZE);
    // 获取整个发送过程需要发送的帧长度
    // 多一个头帧和尾帧
    this->_ext_size = this->_size + 2;
  }

  ~iBinaryStream() {
    if (infile.is_open())
      infile.close();
  }

  Data *next() override {
    if (_cur == 0) {
      // 第一个帧
      first();
    } else if (_cur == _ext_size - 1) {
      // 最后一个帧
      end();
    } else {
      // 中间的数据帧
      mid();
    }
    _cur = _cur + 1;
    return &ret;
  }
};

class oBinaryStream : public BinaryStream {
protected:
  std::ofstream outfile;
  void first(Data *data) override {
    _frame *_fp = (_frame *)data->buf;
    if (_fp->_type != FRAME_TYPE_START)
      throw std::runtime_error("oBinaryStream::first frame type error");
    if (_fp->_id != 0)
      throw std::runtime_error("oBinaryStream::first frame id error");
    this->_size = _fp->_length;
    this->_ext_size = this->_size + 2;
  }

  void end(Data *data) override {
    _frame *_fp = (_frame *)data->buf;
    if (_fp->_type != FRAME_TYPE_END)
      throw std::runtime_error("oBinaryStream::end frame type error");
    if (_fp->_id != this->ext_size() - 1)
      throw std::runtime_error("oBinaryStream::end frame id error");
  }

  void mid(Data *data) override {
    _frame *_fp = (_frame *)data->buf;
    if (_fp->_type != FRAME_TYPE_DATA)
      throw std::runtime_error("oBinaryStream::mid frame type error");
    outfile.write(_fp->buf, _fp->_length);
  }

public:
  oBinaryStream(std::string file_path) : BinaryStream(file_path) {
    outfile = std::ofstream(file_path, ios::out | ios::binary);
    if (!outfile)
      throw std::runtime_error(
          "oBinaryStream::oBinaryStream failed to open file");
  }

  ~oBinaryStream() {
    if (outfile.is_open())
      outfile.close();
  }

  void next(Data *data) override {
    if (_cur == 0) {
      // 第一个帧
      first(data);
    } else if (_cur == _ext_size - 1) {
      // 最后一个帧
      end(data);
    } else {
      // 中间的数据帧
      mid(data);
    }
    _cur++;
  }
};

} // namespace Socket

#endif // _SOCKET_HPP_
