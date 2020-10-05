#ifndef EPOLL_SERVER_TIME_WHEEL_SCHEDULER_H_
#define EPOLL_SERVER_TIME_WHEEL_SCHEDULER_H_

#include <mutex>
#include <vector>
#include <thread>
#include <unordered_set>

#include "epoll_server/time_wheel.h"

namespace epoll_server {

class TimeWheelScheduler {
public:
  explicit TimeWheelScheduler(uint32_t timer_step_ms = 50);

  // Return timer id. Return 0 if the timer creation fails.
  uint32_t CreateTimerAt(int64_t when_ms, const TimerTask& task);
  uint32_t CreateTimerAfter(int64_t delay_ms, const TimerTask& task);
  uint32_t CreateTimerEvery(int64_t interval_ms, const TimerTask& task);

  void CancelTimer(uint32_t timer_id);

  bool Start(const std::function<void(TimerPtr)>& handler);
  void Stop();

  void AppendTimeWheel(uint32_t scales, uint32_t scale_unit_ms, const std::string& name = "");

  uint32_t timer_step_ms() const {
    return timer_step_ms_;
  }

private:
  void Run();

  TimeWheelPtr GetGreatestTimeWheel();
  TimeWheelPtr GetLeastTimeWheel();

private:
  std::mutex mutex_;
  std::thread thread_;

  bool stop_;
  std::unordered_set<uint32_t> cancel_timer_ids_;

  uint32_t timer_step_ms_;
  std::vector<TimeWheelPtr> time_wheels_;

  std::function<void(TimerPtr)> handler_;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_TIME_WHEEL_SCHEDULER_H_
