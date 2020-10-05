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
#include "epoll_server/time_wheel_scheduler.h"
#include "epoll_server/timer.h"

namespace epoll_server {

// The network I/O are in the same thread. The business logic is handled in thread pool.

class Server {
public:
  Server();

  ~Server() = default;

  void Init(const std::string& config_path = "conf/config.json");

  bool Start();

  void AddRouter(uint16_t msg_code, RouterPtr router);

  void set_on_connected(const std::function<void(Connection*)>& on_connected) {
    on_connected_ = on_connected;
  }

  void set_on_disconnected(const std::function<void(Connection*)>& on_disconnected) {
    on_disconnected_ = on_disconnected;
  }

  // Return timer id. Return 0 if the timer creation fails.
  uint32_t CreateTimerAt(int64_t when_ms, const TimerTask& task);
  uint32_t CreateTimerAfter(int64_t delay_ms, const TimerTask& task);
  uint32_t CreateTimerEvery(int64_t interval_ms, const TimerTask& task);

  void CancelTimer(uint32_t timer_id);

private:
  bool Listen();

  bool PollOnce();

  // The accecpt, read and write operations are in the same thread.
  void HandleAccpet(Connection* conn);
  void HandleRead(Connection* conn);
  
  void HandlePendingResponses();

  void HandlePendingTimers();

  // Use thread pool to handle requests.
  void HandleRequest(MessagePtr request);

  void HandleTimeWheelScheduler(TimerPtr timer);

  // Trigger a epoll event and wake up epoll_wait.
  void WakeUp();

  void InitTimeWheelScheduler();

private:
  int acceptor_fd_;
  std::unique_ptr<Connection> acceptor_connection_;

  int wakener_fd_;
  std::unique_ptr<Connection> wakener_connection_;

  Epoller epoller_;

  std::unique_ptr<ConnectionPool> connection_pool_;

  ThreadPool<Message> request_thread_pool_;

  std::mutex pending_response_mutex_;
  std::vector<MessagePtr> pending_responses_;

  std::mutex pending_timer_mutex_;
  std::vector<TimerPtr> pending_timers_;

  TimeWheelScheduler time_wheel_scheduler_;

  // The key is Message Code.
  std::unordered_map<uint16_t, RouterPtr> routers_;

  std::function<void(Connection*)> on_connected_;
  std::function<void(Connection*)> on_disconnected_;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_SERVER_H_
