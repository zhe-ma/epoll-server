#ifndef APP_ROUTER_BASE_H_
#define APP_ROUTER_BASE_H_

#include <string>
#include <memory>

#include "app/message.h"

namespace app {

class RouterBase {
public:
  virtual ~RouterBase() = default;

  // Return response data string bytes.
  virtual std::string HandleRequest(MessagePtr msg) = 0;
};

using RouterPtr = std::shared_ptr<RouterBase>;

}  // namespace app

#endif  // APP_ROUTER_BASE_H_
