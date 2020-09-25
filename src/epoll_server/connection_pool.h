#ifndef EPOLL_SERVER_CONNECTION_POOL_H_
#define EPOLL_SERVER_CONNECTION_POOL_H_

#include <deque>

#include "epoll_server/noncopyable.h"

namespace epoll_server {

class Connection;

class ConnectionPool : private Noncopyable {
public:
  ConnectionPool(size_t size);
  ~ConnectionPool();

  Connection* Get();

  void Release(Connection* conn);

  size_t Size() const;

  bool Empty() const;

private:
  std::deque<Connection*> pool_;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CONNECTION_POOL_H_
