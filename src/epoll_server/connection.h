#ifndef EPOLL_SERVER_CONNECTION_H_
#define EPOLL_SERVER_CONNECTION_H_

#include <string>
#include <functional>

#include "epoll_server/message.h"

struct sockaddr_in;

namespace epoll_server {

class Connection {
public:
  enum Type {
    kTypeAcceptor = 0,
    kTypeWakener,
    kTypeSocket
  };

public:
  Connection(int fd = -1, Type type = kTypeSocket);

  ~Connection();

  void Close();

  void set_fd(int fd) {
    fd_ = fd;
  }

  int fd() const {
    return fd_;
  }

  Type type() const {
    return type_;
  }

  void set_type(Type type) {
    type_ = type;
  }

  uint32_t epoll_events() const {
    return epoll_events_;
  }

  int64_t timestamp() const {
    return timestamp_;
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

  void SetSendData(std::string&& send_data, size_t sended_len);

  void UpdateTimestamp();

  // Return socket fd.
  int HandleAccept(struct sockaddr_in* sock_addr);

  // Inititalize msg if recieved a completed message.
  // Return false if client closed or some read errors occurred.
  bool HandleRead(MessagePtr* msg);

  bool HandleWrite();

  void HandleWakeUp();

  void SetReadEvent(bool enable);
  void SetWriteEvent(bool enable);

private:
  int fd_;
  Type type_;

  uint32_t epoll_events_;

  // Every alive connection has a unique timestamp.
  int64_t timestamp_;  // Millsecond.

  std::string remote_ip_;
  unsigned short remote_port_;

  std::string recv_header_;
  size_t recv_header_len_;

  std::string recv_data_;
  size_t recv_data_len_;

  std::string send_data_;
  size_t sended_len_;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CONNECTION_H_
