#ifndef EPOLL_SERVER_EPOLLER_H_
#define EPOLL_SERVER_EPOLLER_H_

#include <sys/epoll.h>

namespace epoll_server {

const size_t kMaxEpollEvents = 1024;

class Epoller {
public:
  Epoller();
  ~Epoller();

  bool Create();

  // waiting_ms > 0: Waiting timeout.
  // waiting_ms = 0: Return immediately.
  // waiting_ms = -1: Block until receive evets.
  // Return >= 0: Received events count.
  // Return < 0: Error.
  int Poll(int waiting_ms = -1);

  const struct epoll_event& GetEvent(size_t index) const;

  // Register the target fd on the epoll instance.
  bool Add(int target_fd, uint32_t events, void* ptr);

  // Change the the event of target fd.
  bool Modify(int target_fd, uint32_t events, void* ptr);

private:
  int fd_;
  struct epoll_event events_[kMaxEpollEvents];
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_EPOLLER_H_
