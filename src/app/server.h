#ifndef APP_SERVER_H_
#define APP_SERVER_H_

namespace app {

class Connection;

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

private:
  bool Listen();

  bool UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write);

private:
  unsigned short port_;

  int listen_fd_;
  int epoll_fd_;
};

}  // namespace app

#endif  // APP_SERVER_H_
