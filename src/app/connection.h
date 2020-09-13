#ifndef APP_CONNECTION_H_
#define APP_CONNECTION_H_

#include <string>
#include <functional>

namespace app {

class Connection {
public:
  Connection(int socket_fd);

  int socket_fd() const {
    return socket_fd_;
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

  void set_read_hander(const std::function<void()>& read_handler) {
    read_handler_ = read_handler;
  }

  void set_write_hander(const std::function<void()>& write_handler) {
    write_handler_ = write_handler;
  }

  void HandleRead();
  void HandleWrite();

private:
  int socket_fd_;

  std::string remote_ip_;
  unsigned short remote_port_;

  std::function<void()> read_handler_;
  std::function<void()> write_handler_;
};

}  // namespace app

#endif  // APP_CONNECTION_H_
