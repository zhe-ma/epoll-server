#ifndef EPOLL_EPOLL_SERVER_TIMER_H_
#define EPOLL_EPOLL_SERVER_TIMER_H_

#include <cstdint>
#include <functional>
#include <memory>

namespace epoll_server {

typedef std::function<void()> TimerTask;

class Timer {
public:
  Timer(uint32_t id, int64_t when_ms, int64_t interval_ms, const TimerTask& task);

  void Run();

  uint32_t id() const {
    return id_;
  }

  int64_t when_ms() const {
    return when_ms_;
  }

  bool repeated() const {
    return repeated_;
  }

  void UpdateWhenTime() {
    when_ms_ += interval_ms_;
  }

private:
  uint32_t id_;
  TimerTask task_;
  int64_t when_ms_;
  uint32_t interval_ms_;
  bool repeated_;
};

using TimerPtr = std::shared_ptr<Timer>;

}  // namespace epoll_server

#endif  // EPOLL_EPOLL_SERVER_TIMER_H_
