#include "app/epoller.h"

#include <sys/unistd.h>

#include "app/logging.h"

namespace app {

Epoller::Epoller() : fd_(-1) {
}

Epoller::~Epoller() {
  if (fd_ == -1) {
    return;
  }

  close(fd_);
  fd_ = -1;
}

bool Epoller::Create() {
  fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (fd_ == -1) {
    SPDLOG_ERROR("Failed to create epoll.");
    return false;
  }

  return true;
}

int Epoller::Poll(int waiting_ms) {
  int n = epoll_wait(fd_, events_, kMaxEpollEvents, waiting_ms);
  if (n == -1 ) {
    // EINTR: epoll_wait is interrupted by other high level system call.
    // E.g. The error occurs if use GDB to debug. The error can be ignored.
    if (errno == EINTR) {
      return 0;
    }

    SPDLOG_ERROR("Failed to epoll wait. Error: {}-{}.", errno, strerror(errno));
    return -1;
  }

  // n==0 means timeout. But if waiting_ms is -1, the epoll_wait will always block when no event.
  if (n == 0 && waiting_ms == -1) {
    SPDLOG_ERROR("No events receieved when blocking epoll_wait timeout.");
    return -1;
  }

  return n;
}

const struct epoll_event& Epoller::GetEvent(size_t index) const {
  assert(index < kMaxEpollEvents);
  return events_[index];
}

}  // namespace app
