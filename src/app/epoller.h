#ifndef APP_EPOLLER_H_
#define APP_EPOLLER_H_

#include <sys/epoll.h>

namespace app {

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

private:
  int fd_;
  struct epoll_event events_[kMaxEpollEvents];
};

}  // namespace app

#endif  // APP_EPOLLER_H_
