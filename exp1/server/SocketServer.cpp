#include "Socket.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

class SocketServer {
private:
  int server_fd;                           // 服务器socket文件描述符
  std::vector<std::thread> client_threads; // 存储所有客户端处理线程
  std::mutex cout_mutex; // 用于同步控制台输出的互斥锁
  bool running;          // 服务器运行状态标志
  bool file_mode;
  std::string send_file_path;

  void mutex_print(std::string msg) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << msg << std::endl;
  }

  void send_ack(int client_socket, int frame_id) {
    Socket::Data _data;
    _data.buf->_id = frame_id;
    _data.buf->_type = Socket::FRAME_TYPE_ACK;
    _data.buf->_length = 0;
    _data.size = Socket::FRAME_HEAD_SIZE;
    send_frame(client_socket, &_data);
  }

  void recv_ack(int client_soket) {
    Socket::Data data = recv_frame(client_soket);
    if (data.buf->_type != Socket::FRAME_TYPE_ACK)
      recv_ack(client_soket);
  }

  void send_frame(int client_socket, Socket::Data *_data) {
    send(client_socket, (char *)_data->buf, _data->size, 0);
  }

  Socket::Data recv_frame(int client_socket) {
    Socket::Data data;
    char buffer[1024] = {0};
    int _valread = recv(client_socket, buffer, sizeof(buffer), 0);
    if (_valread <= 0)
      throw std::runtime_error("接收文件过程中出现错误");
    data.from_bytes(buffer, _valread);
    return data;
  }

  int get_frame_status(char *buffer) {
    Socket::_frame *_f = (Socket::_frame *)buffer;
    return _f->_type;
  }

  // 处理单个客户端连接的函数
  void handleClient(int client_socket, std::string client_ip) {
    char buffer[1024] = {0};

    mutex_print("新客户端连接: " + client_ip);

    // 持续处理该客户端的消息
    while (running) {
      // 清空缓冲区
      memset(buffer, 0, sizeof(buffer));

      // 接收客户端消息
      int valread = recv(client_socket, buffer, sizeof(buffer), 0);

      // 如果接收出错或连接关闭，退出循环
      if (valread <= 0) {
        break;
      }

      if (valread == 1) {
        continue;
      }

      if (file_mode) {
        Socket::_frame *_fp = (Socket::_frame *)buffer;
        int recv_type = _fp->_type;
        if (recv_type == Socket::FRAME_TYPE_SEND_DATA) {
          mutex_print("客户端" + client_ip + "发送文件");
          std::string recv_file_path = "recv_" + client_ip;
          Socket::oBinaryStream obs(recv_file_path);
          while (obs.has_next()) {
            Socket::Data data = recv_frame(client_socket);
            mutex_print("[RECV] 收到客户端" + client_ip + "发送的帧" +
                        to_string(data.buf->_id));
            obs.next(&data);
            send_ack(client_socket, data.buf->_id);
          }
        } else if (recv_type == Socket::FRAME_TYPE_REQUEST_DATA) {
          mutex_print("客户端" + client_ip + "请求文件");
          Socket::iBinaryStream ibs(this->send_file_path);
          while (ibs.has_next()) {
            Socket::Data *data = ibs.next();
            mutex_print("[SEND] 向客户端" + client_ip + "发送帧" +
                        to_string(data->buf->_id));
            send_frame(client_socket, data);
            recv_ack(client_socket);
          }
        }
      } else {
        Socket::_frame *_fp = (Socket::_frame *)buffer;
        mutex_print("收到来自" + client_ip + "的消息：" +
                    std::string(_fp->buf));

        // 收到客户端发来的关闭请求
        if (std::string(_fp->buf) == "quit") {
          mutex_print("收到来自客户端" + client_ip + "的关闭请求，正在关闭...");
          break;
        }
        // 发送回声响应
        std::string resp;
        std::cout << "服务器回复客户端" + client_ip + "（请输入）：";
        std::cin >> resp;
        Socket::Data _data;
        _data.buf->_id = 0;
        _data.buf->_length = resp.size();
        _data.buf->_type = Socket::FRAME_TYPE_MSG;
        memcpy(_data.buf->buf, resp.c_str(), resp.size());
        _data.size = Socket::FRAME_HEAD_SIZE + resp.size();
        send_frame(client_socket, &_data);
        mutex_print("回复" + client_ip + "的消息：" + resp);
      }
    }

    // 关闭客户端socket
    close(client_socket);
    mutex_print("客户端断开连接: " + client_ip);
  }

public:
  // 构造函数：初始化服务器
  SocketServer(int port, bool file_mode_, std::string send_file_path_)
      : running(true), file_mode(file_mode_), send_file_path(send_file_path_) {
    // 创建socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      throw std::runtime_error("Socket创建失败");
    }

    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      close(server_fd);
      throw std::runtime_error("设置socket选项失败");
    }

    // 配置服务器地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
    address.sin_port = htons(port);       // 设置端口号

    // 绑定地址和端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      close(server_fd);
      throw std::runtime_error("绑定失败");
    }

    // 开始监听连接请求
    if (listen(server_fd, SOMAXCONN) < 0) {
      close(server_fd);
      throw std::runtime_error("监听失败");
    }

    std::cout << "服务器启动成功，监听端口: " << port << std::endl;
  }

  // 启动服务器，开始接受客户端连接
  void start() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    std::cout << "等待客户端连接..." << std::endl;

    // 主循环：持续接受新的客户端连接
    while (running) {
      // 接受新的客户端连接
      int client_socket =
          accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
      if (client_socket < 0) {
        std::cerr << "接受连接失败" << std::endl;
        continue;
      }

      // 获取客户端IP地址
      std::string client_id = to_string(this->client_threads.size());

      try {
        // 为新客户端创建处理线程
        // 这里会调用handleClient函数作为新线程的入口点
        client_threads.emplace_back(&SocketServer::handleClient, this,
                                    client_socket, client_id);
      } catch (const std::exception &e) {
        std::cerr << "创建客户端处理线程失败: " << e.what() << std::endl;
        close(client_socket);
      }
    }
  }

  // 停止服务器
  void stop() {
    running = false;
    close(server_fd);

    // 等待所有客户端线程结束
    for (auto &thread : client_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  // 析构函数：确保资源被正确释放
  ~SocketServer() {
    if (running) {
      stop();
    }
  }
};

SocketServer *server = nullptr;

void signalHandler(int signal) {
  if (signal == SIGINT && server) {
    cout << "Caught Ctrl+C Exiting..." << endl;
    server->stop();
    delete server;
    exit(0);
  }
}

int main() {
  try {
    // 创建服务器实例，监听8080端口
    cout << "[1] : send msg" << endl
         << "[2] : send file" << endl
         << "Please input: ";
    int n;
    cin >> n;
    if (n == 2) {
      server = new SocketServer(6028, true, "file2");
    } else if (n == 1) {
      server = new SocketServer(6028, false, "file2");
    }
    // 启动服务器
    server->start();
  } catch (const std::exception &e) {
    std::cerr << "错误: " << e.what() << std::endl;
    return 1;
  }
  delete server;
  return 0;
}