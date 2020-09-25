#ifndef EPOLL_SERVER_NONCOPYABLE_H_
#define EPOLL_SERVER_NONCOPYABLE_H_

namespace epoll_server {

class Noncopyable {
public:
  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator=(const Noncopyable&) = delete;

protected:
  Noncopyable() = default;
  ~Noncopyable() = default;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CONNECTION_POLL_H_
