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

#include <sys/time.h>
#include <sys/select.h>

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

  void set_less_time_wheel(TimeWheel* less_time_wheel) {
    less_time_wheel_ = less_time_wheel;
  }

  void set_greater_time_wheel(TimeWheel* greater_time_wheel) {
    greater_time_wheel_ = greater_time_wheel;
  }

  void AddTimer(TimerPtr timer) {
    if (!timer) {
      return;
    }

    int64_t now = GetNowTimestamp();
    int64_t diff = timer->when_ms() - now;

    if (diff >= scale_unit_ms_) {
      size_t n = (current_index_ + diff / scale_unit_ms_) % scales_;
      slots_[n].push_back(timer);
      return;
    }

    if (less_time_wheel_ == nullptr) {
      slots_[current_index_].push_back(timer);
      return;
    }

    less_time_wheel_->AddTimer(timer);
  }

  void Increase() {
    ++current_index_;
    if (current_index_ < scales_) {
      return;
    }

    current_index_ = current_index_ % scales_;
    if (greater_time_wheel_ != nullptr) {
      greater_time_wheel_->Increase();
      std::list<TimerPtr> slot = std::move(greater_time_wheel_->GetAndClearCurrentSlot());
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

  TimeWheel* less_time_wheel_;  // Less scale unit.
  TimeWheel* greater_time_wheel_;  // Greater scale unit.
};

TimeWheel::TimeWheel(uint32_t scales, uint32_t scale_unit_ms)
    : current_index_(0)
    , scales_(scales)
    , scale_unit_ms_(scale_unit_ms)
    , slots_(scales)
    , greater_time_wheel_(nullptr)
    , less_time_wheel_(nullptr) {
}

static uint32_t s_inc_id = 1;

class TimeWheelScheduler {
public:
  TimeWheelScheduler();

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

  TimeWheel time_wheel1_;  // Level one time wheel. The least scale unit.
  TimeWheel time_wheel2_;
  TimeWheel time_wheel3_;
  TimeWheel time_wheel4_;
  TimeWheel time_wheel5_;  // Level five time wheel. The greatest scale unit.
};


static const size_t kTimeWheel1Scales = 256;
static const size_t kOtherTimeWheelScales = 64;

//(2^8 * 2^6 * 2^6 * 2^6 *2^6)=2^32

// TODO: Timer_wheel, use level time 1 to add timer.
TimeWheelScheduler::TimeWheelScheduler()
    : time_wheel1_(kTimeWheel1Scales, 1)
    , time_wheel2_(kOtherTimeWheelScales, kTimeWheel1Scales * time_wheel1_.scale_unit_ms())
    , time_wheel3_(kOtherTimeWheelScales, kOtherTimeWheelScales * time_wheel2_.scale_unit_ms())
    , time_wheel4_(kOtherTimeWheelScales, kOtherTimeWheelScales * time_wheel3_.scale_unit_ms())
    , time_wheel5_(kOtherTimeWheelScales, kOtherTimeWheelScales * time_wheel4_.scale_unit_ms()) {

  time_wheel5_.set_less_time_wheel(&time_wheel4_);

  time_wheel4_.set_less_time_wheel(&time_wheel3_);
  time_wheel4_.set_greater_time_wheel(&time_wheel5_);

  time_wheel3_.set_less_time_wheel(&time_wheel2_);
  time_wheel3_.set_greater_time_wheel(&time_wheel4_);

  time_wheel2_.set_less_time_wheel(&time_wheel1_);
  time_wheel2_.set_greater_time_wheel(&time_wheel3_);

  time_wheel1_.set_greater_time_wheel(&time_wheel2_);
}

/*seconds: the seconds; mseconds: the micro seconds*/
void setTimer(int seconds, int mseconds) {
 struct timeval temp;

 temp.tv_sec = seconds;
 temp.tv_usec = mseconds;

 select(0, NULL, NULL, NULL, &temp);

 return;
}

// TODO: how to exit.
void TimeWheelScheduler::Run() {
  int i = 0;
  while (true) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    setTimer(0,1000);
    //std::lock_guard<std::mutex> lock(mutex_);
    time_wheel1_.Increase();
    ++i;
    auto c = time_wheel1_.GetAndClearCurrentSlot();
    for (auto t : c) {
      t->Run();
      std::cout << i << std::endl;

    }

  }
}


uint32_t TimeWheelScheduler::RunAt(int64_t when_ms, const TimerTask& handler) {
  std::lock_guard<std::mutex> lock(mutex_);

  ++s_inc_id;
  time_wheel5_.AddTimer(std::make_shared<Timer>(s_inc_id, when_ms, 0, handler));

  return s_inc_id;
}

uint32_t TimeWheelScheduler::RunAfter(uint32_t delay_ms, const TimerTask& handler) {
  int64_t when = GetNowTimestamp() + static_cast<int64_t>(delay_ms);
  return RunAt(when, handler);
}

uint32_t TimeWheelScheduler::RunEvery(uint32_t interval_ms, const TimerTask& handler) {
  std::lock_guard<std::mutex> lock(mutex_);

  ++s_inc_id;
  time_wheel5_.AddTimer(std::make_shared<Timer>(s_inc_id, GetNowTimestamp(), interval_ms, handler));

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
