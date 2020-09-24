#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include <memory>
#include <vector>
#include <unordered_map>

#include "app/connection_pool.h"
#include "app/epoller.h"
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

private:
  bool Listen();

  bool PollOnce();

  void HandleAccpet(Connection* conn);
  void HandleRead(Connection* conn);

  void HandleRequest(MessagePtr request);

  void HandlePendingResponses();

  // Trigger a epoll event and wake up epoll_wait.
  void WakeUp();

private:
  unsigned short port_;

  int acceptor_fd_;
  int event_fd_;

  Epoller epoller_;

  ConnectionPool connection_pool_;

  ThreadPool<Message> request_thread_pool_;

  std::mutex pending_response_mutex_;
  std::vector<MessagePtr> pending_responses_;

  // The key is Message Code.
  std::unordered_map<uint16_t, RouterPtr> routers_;
};

}  // namespace app

#endif  // APP_SERVER_H_
