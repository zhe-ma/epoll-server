#ifndef APP_CONNECTION_H_
#define APP_CONNECTION_H_

#include <atomic>
#include <string>
#include <functional>

#include "app/message.h"

struct sockaddr_in;

namespace app {

class Connection {
public:
  Connection();

  ~Connection();

  void Close();

  void set_fd(int fd) {
    fd_ = fd;
  }

  int fd() const {
    return fd_;
  }

  void set_is_listen_socket(bool is_listen_socket) {
    is_listen_socket_ = is_listen_socket;
  }

  bool is_listen_socket() const {
    return is_listen_socket_;
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

  void UpdateTimestamp();

  int64_t GetTimestamp() const;

  // Return socket fd.
  int HandleAccept(struct sockaddr_in* sock_addr);

  // Inititalize msg if recieved a completed message.
  // Return false if client closed or some read errors occurred.
  bool HandleRead(MessagePtr* msg);

  void HandleWrite();

private:
  int Recv(char* buf, size_t buf_len);

private:
  int fd_;
  bool is_listen_socket_;

  std::atomic<int64_t> timestamp_;  // Millsecond.

  std::string remote_ip_;
  unsigned short remote_port_;

  std::string recv_header_;
  size_t recv_header_len_;

  std::string recv_data_;
  size_t recv_data_len_;
};

}  // namespace app

#endif  // APP_CONNECTION_H_
