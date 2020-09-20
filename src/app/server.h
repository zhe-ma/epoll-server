#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include <vector>
#include <unordered_map>

#include <sys/epoll.h>

#include "app/connection_pool.h"
#include "app/thread_pool.h"
#include "app/router_base.h"
#include "app/message.h"

namespace app {

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

  void AddRouter(uint16_t msg_code, RouterPtr router);

  bool UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write);

private:
  bool Listen();

  bool PollOnce(int waiting_ms);

  void HandleAccpet(Connection* conn);
  void HandleRead(Connection* conn);

  void HandleRequest(MessagePtr request);

private:
  unsigned short port_;

  int listen_fd_;
  int epoll_fd_;

  std::vector<struct epoll_event> epoll_events_;

  ConnectionPool connection_pool_;
  ThreadPool<Message> thread_pool_;

  // The key is Message Code.
  std::unordered_map<uint16_t, RouterPtr> routers_;
};

}  // namespace app

#endif  // APP_SERVER_H_
