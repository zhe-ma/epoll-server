#ifndef APP_THREAD_POOL_H_
#define APP_THREAD_POOL_H_

#include <thread>
#include <vector>
#include <functional>
#include <memory>

#include "app/thread_safe_queue.h"

namespace app {

template <class T>
class ThreadPool {
public:
  using TPtr = std::shared_ptr<T>;

  void Start(size_t thread_size, std::function<void(TPtr t)>&& handler) {
    handler_ = std::move(handler);

    for (size_t i = 0; i < thread_size; i++) {
      threads_.emplace_back(std::bind(&ThreadPool::WorkRoutine, this));
    }
  }

  void StopAndWait() {
    queue_.Clear();
    queue_.Push(TPtr());

    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }

    threads_.clear();
    queue_.Clear();
  }

  void Add(TPtr&& t) {
    if (!t) {
      return;
    }

    queue_.Push(std::move(t));
  }

private:
  void WorkRoutine() {
    for (;;) {
      TPtr t = queue_.PopOrWait();
      if (!t) {
        queue_.Push(TPtr());
        break;
      }

      handler_(t);
    }
  }

private:
  std::vector<std::thread> threads_;
  ThreadSafeQueue<TPtr> queue_;
  std::function<void(TPtr)> handler_;
};

}  // namespace app

#endif  // APP_THREAD_POOL_H_
