#ifndef APP_CONNECTION_POOL_H_
#define APP_CONNECTION_POOL_H_

#include <deque>

#include "app/noncopyable.h"

namespace app {

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
  std::deque<Connection*> poll_;
};

}  // namespace app

#endif  // APP_CONNECTION_POOL_H_
