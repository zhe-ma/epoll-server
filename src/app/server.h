#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include <sys/epoll.h>

namespace app {

class Connection;

const int MAX_EPOLL_EVENTS = 512;

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

  bool UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write);

private:
  bool Listen();

  bool PollOnce(int waiting_ms);

private:
  unsigned short port_;

  int listen_fd_;
  int epoll_fd_;

  struct epoll_event epoll_events_[MAX_EPOLL_EVENTS]; 
};

}  // namespace app

#endif  // APP_SERVER_H_
