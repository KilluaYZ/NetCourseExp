#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <iostream>
#include <sstream>
#include <exception>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <cmath>
#include <cstring>
#include <vector>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;

namespace Socket
{

    const int FRAME_BUF_SIZE = 1016;
    const int FRAME_HEAD_SIZE = 8;
    const int FRAME_SIZE = 1024;
    const int FRAME_TYPE_START = 1;
    const int FRAME_TYPE_END = 2;
    const int FRAME_TYPE_DATA = 3;
    const int FRAME_TYPE_ACK = 4;
    const int FRAME_TYPE_REQUEST_DATA = 5;
    const int FRAME_TYPE_SEND_DATA = 6;

    typedef int Socket;
    typedef string Ip;
    typedef unsigned int Port;

    class MException
    {
    private:
        string _message;

    public:
        MException(string error) { this->_message = error; }
        virtual const char *what() { return this->_message.c_str(); }
    };

    struct _frame
    {
        uint32_t _id;
        uint16_t _type;
        uint16_t _length;
        char buf[FRAME_BUF_SIZE];
    };

    class Data
    {
    public:
        int size;
        _frame *buf;
        Data()
        {
            this->size = 0;
            this->buf = (_frame *)malloc(sizeof(char) * FRAME_SIZE);
        }

        ~Data()
        {
            free(this->buf);
        }

        void copy(Data *data)
        {
            if (data == nullptr)
                throw MException("Error while copying Data");
            this->size = data->size;
            memcpy(this->buf, data->buf, sizeof(_frame));
        }
    };

    typedef struct
    {
        Ip ip;
        Port port;
    } Address;

    struct sockaddr_in *to_sockaddr(Address *a)
    {
        struct sockaddr_in *ret;

        ret = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
        ret->sin_family = AF_INET;
        inet_aton(a->ip.c_str(), &(ret->sin_addr));
        ret->sin_port = htons(a->port);

        return ret;
    }

    Address *from_sockaddr(struct sockaddr_in *address)
    {
        Address *ret;

        ret = (Address *)malloc(sizeof(Address));
        ret->ip = inet_ntoa(address->sin_addr);
        ret->port = ntohs(address->sin_port);

        return ret;
    }

    class TCPServer
    {
    public:
        Socket _server_socket_id;
        Socket _client_socket_id;
        Data *data;

    public:
        TCPServer()
        {
            _server_socket_id = socket(AF_INET, SOCK_STREAM, 0);
            if (_server_socket_id == -1)
                throw MException("Socket creation failed");
            this->data = new Data();
        }

        ~TCPServer()
        {
            delete this->data;
        }

        void tcp_bind(Ip ip, Port port)
        {
            sockaddr_in server_address;
            memset(&server_address, 0, sizeof(server_address));

            // 配置服务器地址
            server_address.sin_family = AF_INET;
            server_address.sin_port = htons(port);
            server_address.sin_addr.s_addr = INADDR_ANY;

            // 将Socket与服务器绑定
            if (bind(_server_socket_id, (sockaddr *)&server_address, sizeof(server_address)) == -1)
                throw MException("Binding failed");
        }

        int read_stat()
        {
            tcp_recv();
            return this->data->buf->_type;
        }

        void tcp_listen()
        {
            // 只允许一个client连接
            if (listen(this->_server_socket_id, 1) == -1)
                throw MException("listen error");
        }

        void tcp_accept()
        {
            sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            // 接受客户端的连接
            this->_client_socket_id = accept(this->_server_socket_id, (sockaddr *)&client_addr, &client_addr_len);
            if (this->_client_socket_id == -1)
                throw MException("Accepting client connection failed");
            cout << "Client connected!" << endl;
        }

        void tcp_recv()
        {
            int recv_size = recv(this->_client_socket_id, this->data->buf, sizeof(_frame), 0);
            this->data->size = recv_size;
        }

        void tcp_send()
        {
            send(this->_client_socket_id, (char *)this->data->buf, sizeof(_frame), 0);
        }
    };

    class BinaryStream
    {
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
        BinaryStream(std::string file_path)
        {
            this->file_path = file_path;
            this->_size = 0;
            this->_cur = 0;
            this->_ext_size = 0;
            this->_byte_size = 0;
        };
        virtual Data *next() { return nullptr; };
        virtual void next(Data *data) {};
        virtual bool has_next()
        {
            if (this->_ext_size == 0)
                return true;
            if (this->_cur < this->_ext_size)
                return true;
            return false;
        }
        virtual int size()
        {
            return this->_size;
        }
        virtual int ext_size()
        {
            return this->_ext_size;
        }
    };

    class iBinaryStream : public BinaryStream
    {
    protected:
        std::ifstream infile;
        Data ret;
        Data *first() override
        {
            _frame first_frame;
            first_frame._id = 0;
            first_frame._type = FRAME_TYPE_START;
            first_frame._length = size();
            ret.size = FRAME_HEAD_SIZE;
            memcpy((void *)ret.buf, (void *)&first_frame, ret.size);
            return &ret;
        }

        Data *end() override
        {
            _frame end_frame;
            end_frame._id = ext_size() - 1;
            end_frame._type = FRAME_TYPE_END;
            end_frame._length = 0;
            ret.size = FRAME_HEAD_SIZE;
            memcpy((void *)ret.buf, (void *)&end_frame, ret.size);
            return &ret;
        }

        Data *mid() override
        {
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
        iBinaryStream(std::string file_path) : BinaryStream(file_path)
        {
            infile = std::ifstream(file_path, ios::binary);
            if (!infile)
            {
                throw MException("Failed to open file.");
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

        ~iBinaryStream()
        {
            if (infile.is_open())
                infile.close();
        }

        Data *next() override
        {
            if (_cur == 0)
            {
                // 第一个帧
                first();
            }
            else if (_cur == _ext_size - 1)
            {
                // 最后一个帧
                end();
            }
            else
            {
                // 中间的数据帧
                mid();
            }
            _cur = _cur + 1;
            return &ret;
        }
    };

    class oBinaryStream : public BinaryStream
    {
    protected:
        std::ofstream outfile;
        void first(Data *data) override
        {
            _frame *_fp = (_frame *)data->buf;
            if (_fp->_type != FRAME_TYPE_START)
                throw MException("oBinaryStream::first frame type error");
            if (_fp->_id != 0)
                throw MException("oBinaryStream::first frame id error");
            this->_size = _fp->_length;
            this->_ext_size = this->_size + 2;
        }

        void end(Data *data) override
        {
            _frame *_fp = (_frame *)data->buf;
            if (_fp->_type != FRAME_TYPE_END)
                throw MException("oBinaryStream::end frame type error");
            if (_fp->_id != this->ext_size() - 1)
                throw MException("oBinaryStream::end frame id error");
        }

        void mid(Data *data) override
        {
            _frame *_fp = (_frame *)data->buf;
            if (_fp->_type != FRAME_TYPE_DATA)
                throw MException("oBinaryStream::mid frame type error");
            outfile.write(_fp->buf, _fp->_length);
        }

    public:
        oBinaryStream(std::string file_path) : BinaryStream(file_path)
        {
            outfile = std::ofstream(file_path, ios::out | ios::binary);
            if (!outfile)
                throw MException("oBinaryStream::oBinaryStream failed to open file");
        }

        ~oBinaryStream()
        {
            if (outfile.is_open())
                outfile.close();
        }

        void next(Data *data) override
        {
            if (_cur == 0)
            {
                // 第一个帧
                first(data);
            }
            else if (_cur == _ext_size - 1)
            {
                // 最后一个帧
                end(data);
            }
            else
            {
                // 中间的数据帧
                mid(data);
            }
            _cur++;
        }
    };

    class MFileServer
    {
    private:
        TCPServer tcp_server;
        void ack(uint32_t _id)
        {
            Data tmp_data;
            tmp_data.buf->_id = _id;
            tmp_data.buf->_type = FRAME_TYPE_ACK;
            tmp_data.size = FRAME_HEAD_SIZE;
            tcp_server.data->copy(&tmp_data);
            tcp_server.tcp_send();
        }

    public:
        MFileServer() {};
        ~MFileServer() {};

        void start_service(string src_file_path, string dst_file_path)
        {
            tcp_server.tcp_bind("127.0.0.1", 20000);
            tcp_server.tcp_listen();
            tcp_server.tcp_accept();

            int recv_type = tcp_server.read_stat();
            while (recv_type == FRAME_TYPE_SEND_DATA || recv_type == FRAME_TYPE_REQUEST_DATA)
            {
                if (recv_type == FRAME_TYPE_SEND_DATA)
                {
                    oBinaryStream obs(dst_file_path);
                    while (obs.has_next())
                    {
                        tcp_server.tcp_recv();
                        obs.next(tcp_server.data);
                        // send ack after receive a package
                        ack(tcp_server.data->buf->_id);
                    }
                }
                else if (recv_type == FRAME_TYPE_REQUEST_DATA)
                {
                    iBinaryStream ibs(src_file_path);
                    while (ibs.has_next())
                    {
                        Data *data = ibs.next();
                        tcp_server.data->copy(data);
                        tcp_server.tcp_send();
                        // check if client send ack
                        recv_type = tcp_server.read_stat();
                        if (recv_type != FRAME_TYPE_ACK)
                            throw MException("Error frame type is not ack");
                    }
                }
                recv_type = tcp_server.read_stat();
            }
        }
    };
}

#endif // _SOCKET_HPP_
