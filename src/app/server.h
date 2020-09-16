#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include <vector>

#include <sys/epoll.h>

#include "app/connection_pool.h"

namespace app {

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

  bool UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write);

private:
  bool Listen();

  bool PollOnce(int waiting_ms);

  void HandleAccpet(Connection* conn);
  void HandleRead(Connection* conn);

private:
  unsigned short port_;

  int listen_fd_;
  int epoll_fd_;

  std::vector<struct epoll_event> epoll_events_;

  ConnectionPool connection_pool_;
};

}  // namespace app

#endif  // APP_SERVER_H_
