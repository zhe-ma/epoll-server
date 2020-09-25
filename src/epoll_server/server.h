#ifndef EPOLL_SERVER_SERVER_H_
#define EPOLL_SERVER_SERVER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "epoll_server/connection.h"
#include "epoll_server/connection_pool.h"
#include "epoll_server/epoller.h"
#include "epoll_server/thread_pool.h"
#include "epoll_server/router_base.h"
#include "epoll_server/message.h"

namespace epoll_server {

// The network I/O are in the same thread. The business logic is handled in thread pool.

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

  void AddRouter(uint16_t msg_code, RouterPtr router);

private:
  bool Listen();

  bool PollOnce();

  // The accecpt, read and write operations are in the same thread.
  void HandleAccpet(Connection* conn);
  void HandleRead(Connection* conn);
  
  void HandlePendingResponses();

  // Use thread pool to handle requests.
  void HandleRequest(MessagePtr request);

  // Trigger a epoll event and wake up epoll_wait.
  void WakeUp();

private:
  unsigned short port_;

  int acceptor_fd_;
  std::unique_ptr<Connection> acceptor_connection_;

  int wakener_fd_;
  std::unique_ptr<Connection> wakener_connection_;

  Epoller epoller_;

  ConnectionPool connection_pool_;

  ThreadPool<Message> request_thread_pool_;

  std::mutex pending_response_mutex_;
  std::vector<MessagePtr> pending_responses_;

  // The key is Message Code.
  std::unordered_map<uint16_t, RouterPtr> routers_;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_SERVER_H_
