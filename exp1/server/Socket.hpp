/*
 * Socket.hpp
 * This file is part of VallauriSoft
 *
 * Copyright (C) 2012 - Comina, gnuze
 *
 * VallauriSoft is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VallauriSoft is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with VallauriSoft. If not, see <http://www.gnu.org/licenses/>.
 */

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

    typedef struct
    {
        Address address;
        Data data;
    } Datagram;

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

    class Exception
    {
    private:
        string _message;

    public:
        Exception(string error) { this->_message = error; }
        virtual const char *what() { return this->_message.c_str(); }
    };

    class UDP
    {
    private:
        Socket _socket_id;
        bool _binded;

    public:
        UDP(void);
        ~UDP(void);
        void close(void);
        void bind(Port port);
        void send(Ip ip, Port port, Data *data);
        Datagram receive();
    };

    UDP::UDP(void)
    {
        this->_socket_id = socket(AF_INET, SOCK_DGRAM, 0);
        if (this->_socket_id == -1)
            throw Exception("[Constructor] Cannot create socket");
        this->_binded = false;
    }

    UDP::~UDP(void)
    {
    }

    void UDP::close(void)
    {
        shutdown(this->_socket_id, SHUT_RDWR);
    }

    void UDP::bind(Port port)
    {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (this->_binded)
        {
            this->close();
            this->_socket_id = socket(AF_INET, SOCK_DGRAM, 0);
        }
        // ::bind() calls the function bind() from <arpa/inet.h> (outside the namespace)
        if (::bind(this->_socket_id, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) == -1)
        {
            stringstream error;
            error << "[listen_on_port] with [port=" << port << "] Cannot bind socket";
            throw Exception(error.str());
        }

        this->_binded = true;
    }

    void UDP::send(Ip ip, Port port, Data *data)
    {
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        inet_aton(ip.c_str(), &address.sin_addr);
        cout << "[Send] id: " << ((_frame *)data->buf)->_id << " type: " << ((_frame *)data->buf)->_type << " length: " << ((_frame *)data->buf)->_length << endl;
        if (sendto(this->_socket_id, (void *)data->buf, data->size, 0, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) == -1)
        {
            stringstream error;
            error << "[send] with [ip=" << ip << "] [port=" << port << "] Cannot send";
            throw Exception(error.str());
        }
    }

    Datagram UDP::receive()
    {
        int size = sizeof(struct sockaddr_in);
        char *buffer = (char *)malloc(sizeof(char) * FRAME_SIZE);
        struct sockaddr_in address;
        Datagram ret;
        int recv_size = recvfrom(this->_socket_id, (void *)buffer, FRAME_SIZE, 0, (struct sockaddr *)&address, (socklen_t *)&size);
        if (recv_size == -1)
            throw Exception("[receive] Cannot receive");

        // ret.data.buf = buffer;
        memcpy(ret.data.buf, buffer, recv_size);
        ret.data.size = recv_size;
        cout << "[Recv] id: " << ((_frame *)ret.data.buf)->_id << " type: " << ((_frame *)ret.data.buf)->_type << " length: " << ((_frame *)ret.data.buf)->_length << endl;

        ret.address.ip = inet_ntoa(address.sin_addr);
        ret.address.port = ntohs(address.sin_port);

        free(buffer);

        return ret;
    }

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
                throw Exception("Failed to open file.");
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
                throw Exception("oBinaryStream::first frame type error");
            if (_fp->_id != 0)
                throw Exception("oBinaryStream::first frame id error");
            this->_size = _fp->_length;
            this->_ext_size = this->_size + 2;
        }

        void end(Data *data) override
        {
            _frame *_fp = (_frame *)data->buf;
            if (_fp->_type != FRAME_TYPE_END)
                throw Exception("oBinaryStream::end frame type error");
            if (_fp->_id != this->ext_size() - 1)
                throw Exception("oBinaryStream::end frame id error");
        }

        void mid(Data *data) override
        {
            _frame *_fp = (_frame *)data->buf;
            if (_fp->_type != FRAME_TYPE_DATA)
                throw Exception("oBinaryStream::mid frame type error");
            outfile.write(_fp->buf, _fp->_length);
        }

    public:
        oBinaryStream(std::string file_path) : BinaryStream(file_path)
        {
            outfile = std::ofstream(file_path, ios::out | ios::binary);
            if (!outfile)
                throw Exception("oBinaryStream::oBinaryStream failed to open file");
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
                    throw Exception("FileSocket::send ack error");
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
