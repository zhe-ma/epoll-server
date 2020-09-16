#ifndef APP_MESSAGE_H_
#define APP_REQUEST_H_

#include <memory>

#include "app/connection.h"
#include "app/message.h"

namespace app {

struct Request {
  Connection* conn;
  Message msg;

  Request(Connection* conn_, Message&& msg_)
      : conn(conn_)
      , msg(std::move(msg_)) {
  }
};

using RequestPtr = std::shared_ptr<Request>;

}  // namespace app

#endif  // APP_REQUEST_H_
