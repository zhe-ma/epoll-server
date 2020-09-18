#ifndef APP_THREAD_SAFE_QUEUE_H_
#define APP_THREAD_SAFE_QUEUE_H_

#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>

namespace app {

template <class T>
class ThreadSafeQueue {
public:
  void Push(T&& t) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::forward<T>(t));
    not_empty_cv_.notify_one();
  }

  T PopOrWait() {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_cv_.wait(lock, [this]() {
      return !queue_.empty();
    });

    T t = std::move(queue_.front());
    queue_.pop_front();
    return t;
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
  }

private:
  std::list<T> queue_;

  mutable std::mutex mutex_;
  std::condition_variable not_empty_cv_;
};

}  // namespace app

#endif  // APP_UTAPP_THREAD_SAFE_QUEUE_H_ILS_H_
