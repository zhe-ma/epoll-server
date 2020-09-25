#ifndef EPOLL_SERVER_ROUTER_BASE_H_
#define EPOLL_SERVER_ROUTER_BASE_H_

#include <string>
#include <memory>

#include "epoll_server/message.h"

namespace epoll_server {

class RouterBase {
public:
  virtual ~RouterBase() = default;

  // Return response data string bytes.
  virtual std::string HandleRequest(MessagePtr msg) = 0;
};

using RouterPtr = std::shared_ptr<RouterBase>;

}  // namespace epoll_server

#endif  // EPOLL_SERVER_ROUTER_BASE_H_
