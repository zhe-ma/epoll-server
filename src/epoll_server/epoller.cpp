#include "epoll_server/epoller.h"

#include <sys/unistd.h>

#include "epoll_server/logging.h"

namespace epoll_server {

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

bool Epoller::Add(int target_fd, uint32_t events, void* ptr) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.ptr = ptr;

  if(epoll_ctl(fd_, EPOLL_CTL_ADD, target_fd, &ev) == -1) {
    SPDLOG_ERROR("Failed to add epoll event. Error:{}-{}.", errno, strerror(errno));
    return false;
  }

  return true;
}

bool Epoller::Modify(int target_fd, uint32_t events, void* ptr) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = events;
  ev.data.ptr = ptr;

  if(epoll_ctl(fd_, EPOLL_CTL_MOD, target_fd, &ev) == -1) {
    SPDLOG_ERROR("Failed to update epoll event. Error:{}-{}.", errno, strerror(errno));
    return false;
  }

  return true;
}

}  // namespace epoll_server
