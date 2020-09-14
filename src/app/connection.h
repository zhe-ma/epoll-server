#ifndef APP_CONNECTION_H_
#define APP_CONNECTION_H_

#include <string>
#include <functional>

#include "app/server.h"

namespace app {

class Connection {
public:
  Connection(int socket_fd, Server* server);

  int socket_fd() const {
    return socket_fd_;
  }

  void set_is_listen_socket(bool is_listen_socket) {
    is_listen_socket_ = is_listen_socket;
  }

  void set_remote_ip(const std::string& remote_ip) {
    remote_ip_ = remote_ip;
  }

  const std::string& remote_ip() const {
    return remote_ip_;
  }

  void set_remote_port(unsigned short remote_port) {
    remote_port_ = remote_port;
  }

  unsigned short remote_port() const {
    return remote_port_;
  }

  void HandleRead();
  void HandleWrite();

private:
  void DoRead();
  void DoAccept();

  int Recv(char* buf, size_t buf_len);

private:
  Server* server_;

  int socket_fd_;
  bool is_listen_socket_;

  std::string remote_ip_;
  unsigned short remote_port_;

  std::string recv_header_;
  size_t recv_header_len_;

  std::string recv_data_;
  size_t recv_data_len_;
};

}  // namespace app

#endif  // APP_CONNECTION_H_
