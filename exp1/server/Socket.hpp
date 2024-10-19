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
#include <arpa/inet.h>
#include <vector>

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
        char *buf;
        Data()
        {
            this->size = 0;
            this->buf = (char *)malloc(sizeof(char) * FRAME_SIZE);
        }

        ~Data()
        {
            free(this->buf);
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

    class MException
    {
    private:
        string _message;

    public:
        MException(string error) { this->_message = error; }
        virtual const char *what() { return this->_message.c_str(); }
    };

    class TCPConnect
    {
    private:
        Socket _socket_id;
        bool _binded;

    public:
        TCPConnect()
        {
        }
    };

    class TCPServer
    {
    private:
        int max_client;
        Socket _socket_id;
        Socket _client_socket_id;

    public:
        TCPServer()
        {
            _socket_id = socket(AF_INET, SOCK_STREAM, 0);
            if (_socket_id == -1)
                throw MException("Socket creation failed");
            this->max_client = max_client;
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
            if (bind(_socket_id, (sockaddr *)&server_address, sizeof(server_address)) == -1)
                throw MException("Binding failed");
        }

        void start_service()
        {
            // 只允许一个client连接
            if (listen(_socket_id, 1) == -1)
                throw MException("listen error");

            sockaddr client_addr;
            int client_addr_len = sizeof(client_addr);
            // 接受客户端的连接
            _client_socket_id = accept(this->_socket_id, (sockaddr*)&client_addr, &client_addr);
            if (_client_socket_id == -1) throw MException("Accepting client connection failed");
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
        UDP socket;
        Address client;
        void ack(Ip ip, Port port, uint32_t _id)
        {
            Data tmp_data;
            _frame *_fp = (_frame *)tmp_data.buf;
            _fp->_id = _id;
            _fp->_type = FRAME_TYPE_ACK;
            tmp_data.size = FRAME_HEAD_SIZE;
            socket.send(ip, port, &tmp_data);
        }

    public:
        MFileServer() {};
        ~MFileServer() {};
        void bind(Port port)
        {
            socket.bind(port);
        }

        void send(std::string file_path)
        {
            iBinaryStream ibs(file_path);
            while (ibs.has_next())
            {
                socket.send(client.ip, client.port, ibs.next());
                auto resp = socket.receive();
                if (((_frame *)resp.data.buf)->_type != FRAME_TYPE_ACK)
                    throw MException("FileSocket::send ack error");
            }
        }

        void recv(std::string file_path)
        {
            oBinaryStream obs(file_path);
            while (obs.has_next())
            {
                Datagram dg = socket.receive();
                obs.next(&dg.data);
                ack(client.ip, client.port, ((_frame *)dg.data.buf)->_id);
            }
        }

        int read_status()
        {
            Datagram dg = socket.receive();
            _frame *_fp = (_frame *)dg.data.buf;
            this->client = dg.address;
            return _fp->_type;
        }

        void start_service(string file_path)
        {
            int _resp_status = -1;
            while (_resp_status == read_status())
            {
                if (_resp_status == FRAME_TYPE_REQUEST_DATA)
                {
                    cout << "ip: " << client.ip << "port : " << client.port << " request data";
                    send(file_path);
                }
                else if (_resp_status == FRAME_TYPE_SEND_DATA)
                {
                    cout << "ip: " << client.ip << "port : " << client.port << " send data";
                    recv(file_path);
                }
                else
                {
                    cout << "Error Unknow Operation Type";
                }
            }
        }

        void close()
        {
            socket.close();
        }
    };
}

#endif // _SOCKET_HPP_
