#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <utility>
#include <functional>
#include <algorithm>
#include <cassert>
#include <stdint.h>
#include <chrono>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <thread>


int64_t GetNowTimestamp() {
  using namespace std::chrono;
  auto now = system_clock::now().time_since_epoch();
  return duration_cast<milliseconds>(now).count();
}

typedef std::function<void()> TimerTask;


class Timer {
public:
  Timer(uint32_t id, int64_t when_ms, uint32_t interval_ms, const TimerTask& handler);

  void Run();

  int64_t when_ms() const {
    return when_ms_;
  }

private:
  uint32_t id_;
  TimerTask handler_;
  int64_t when_ms_;
  uint32_t interval_ms_;
  bool repeated_;
};

Timer::Timer(uint32_t id, int64_t when_ms, uint32_t interval_ms, const TimerTask& handler)
    : interval_ms_(interval_ms)
    , repeated_(interval_ms_ > 0)
    , when_ms_(when_ms)
    , id_(id)
    , handler_(handler) {
}

using TimerPtr = std::shared_ptr<Timer>;

void Timer::Run() {
  if (handler_) {
    handler_();
  }
}

class TimeWheel {
public:
  TimeWheel(uint32_t scales, uint32_t scale_unit_ms);


  uint32_t scale_unit_ms() const {
    return scale_unit_ms_;
  }

  uint32_t scales() const {
    return scales_;
  }

  uint32_t current_index() const {
    return current_index_;
  }

  void set_less_level_tw(TimeWheel* less_level_tw) {
    less_level_tw_ = less_level_tw;
  }

  void set_greater_level_tw(TimeWheel* greater_level_tw) {
    greater_level_tw_ = greater_level_tw_;
  }

  void AddTimer(TimerPtr timer) {
    if (!timer) {
      return;
    }

    int64_t diff = timer->when_ms() - GetNowTimestamp();

    if (diff >= scale_unit_ms_) {
      size_t n = (current_index_ + diff / scale_unit_ms_) % scales_;
      slots_[n].push_back(timer);
      return;
    }

    if (less_level_tw_ == nullptr) {
      slots_[current_index_].push_back(timer);
      return;
    }

    less_level_tw_->AddTimer(timer);
  }

  void Increase() {
    ++current_index_;
    if (current_index_ < scales_) {
      return;
    }

    current_index_ = current_index_ % scales_;
    if (greater_level_tw_ != nullptr) {
      greater_level_tw_->Increase();
      std::list<TimerPtr> slot = std::move(greater_level_tw_->GetAndClearCurrentSlot());
      for (TimerPtr timer : slot) {
        AddTimer(timer);
      }
    }
  }

  //std::list<TimerTask> GetDueTasks() {
  //  std::list<TimerPtr> slot = std::move(slots_[current_index_]);
  //  std::list<TimerTask> tasks;
  //  for (const TimerPtr& timer : slot) {
  //    timer->
  //  }
  //}

  std::list<TimerPtr> GetAndClearCurrentSlot() {
    std::list<TimerPtr> slot;
    slot = std::move(slots_[current_index_]);
    return slot;
  }

private:
  uint32_t current_index_;
  uint32_t scales_;
  uint32_t scale_unit_ms_;

  std::vector<std::list<TimerPtr>> slots_;

  TimeWheel* less_level_tw_;  // Less scale unit.
  TimeWheel* greater_level_tw_;  // Greater scale unit.
};

TimeWheel::TimeWheel(uint32_t scales, uint32_t scale_unit_ms)
    : current_index_(0)
    , scales_(scales)
    , scale_unit_ms_(scale_unit_ms)
    , slots_(scales)
    , greater_level_tw_(nullptr)
    , less_level_tw_(nullptr) {
}

static uint32_t s_inc_id = 1;

class TimeWheelScheduler {
public:
  TimeWheelScheduler(uint32_t timer_step_ms = 50);

  // Return timer id. Return 0 if the timer creation fails.
  uint32_t RunAt(int64_t when_ms, const TimerTask& handler);
  uint32_t RunAfter(uint32_t delay_ms, const TimerTask& handler);
  uint32_t RunEvery(uint32_t interval_ms, const TimerTask& handler);

  void CancelTimer(uint32_t timer_id);

  void Run();

private:
  void AddTimer(uint32_t id, int64_t when_ms, uint32_t interval_ms, const TimerTask& handler);

private:
  std::mutex mutex_;

  uint32_t timer_step_ms_;

  TimeWheel millisecond_tw_;  // Level one time wheel. The least scale unit.
  TimeWheel second_tw_;
  TimeWheel minute_tw_;
  TimeWheel hour_tw_;  // Level four time wheel. The greatest scale unit.
};


// TODO: Timer_wheel, use level time 1 to add timer. 误差也是timer_step_ms
TimeWheelScheduler::TimeWheelScheduler(uint32_t timer_step_ms)
    : timer_step_ms_(timer_step_ms)
    , millisecond_tw_(1000 / timer_step_ms, timer_step_ms)
    , second_tw_(60, 1000)
    , minute_tw_(60, 60 * 1000)
    , hour_tw_(24, 60 * 60 * 1000) {

  hour_tw_.set_less_level_tw(&minute_tw_);

  minute_tw_.set_less_level_tw(&second_tw_);
  minute_tw_.set_greater_level_tw(&hour_tw_);

  second_tw_.set_less_level_tw(&millisecond_tw_);
  second_tw_.set_greater_level_tw(&minute_tw_);

  millisecond_tw_.set_greater_level_tw(&second_tw_);
}

// TODO: how to exit.
void TimeWheelScheduler::Run() {
  int i = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::lock_guard<std::mutex> lock(mutex_);
    millisecond_tw_.Increase();
    ++i;
    auto c = millisecond_tw_.GetAndClearCurrentSlot();
    for (auto t : c) {
      t->Run();
      std::cout << i << std::endl;

    }

  }
}


uint32_t TimeWheelScheduler::RunAt(int64_t when_ms, const TimerTask& handler) {
  std::lock_guard<std::mutex> lock(mutex_);

  ++s_inc_id;
  day_tw_.AddTimer(std::make_shared<Timer>(s_inc_id, when_ms, 0, handler));

  return s_inc_id;
}

uint32_t TimeWheelScheduler::RunAfter(uint32_t delay_ms, const TimerTask& handler) {
  int64_t when = GetNowTimestamp() + static_cast<int64_t>(delay_ms);
  return RunAt(when, handler);
}

uint32_t TimeWheelScheduler::RunEvery(uint32_t interval_ms, const TimerTask& handler) {
  std::lock_guard<std::mutex> lock(mutex_);

  ++s_inc_id;
  day_tw_.AddTimer(std::make_shared<Timer>(s_inc_id, GetNowTimestamp(), interval_ms, handler));

  return s_inc_id;
}

void TimeWheelScheduler::CancelTimer(uint32_t timer_id) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
  }
}



int main() {
  TimeWheelScheduler t;
  std::thread tt([&]() {
    t.Run();
  });

  t.RunAfter(10000, [&]() {

    std::cout << "H" << std::endl;
  });

  tt.join();
  //std::cout << 1 << std::endl;
  //for (int i = 0; i < 5000; ++i) {
  //  std::this_thread::sleep_for(std::chrono::milliseconds(2));

  //}
  //std::cout << 1 << std::endl;

  return 0;
}


//#include <sys/time.h>
//#include <sys/select.h>
//#include <time.h>
//#include <stdio.h>
//
///*seconds: the seconds; mseconds: the micro seconds*/
//void setTimer(int seconds, int mseconds) {
//  struct timeval temp;
//
//  temp.tv_sec = seconds;
//  temp.tv_usec = mseconds;
//
//  select(0, NULL, NULL, NULL, &temp);
//  printf("timer\n");
//
//  return;
//}
//
//int main() {
//  int i;
//
//  for (i = 0; i < 100; i++)
//    setTimer(1, 0);
//
//  return 0;
