#ifndef APP_MESSAGE_H_
#define APP_REQUEST_H_

#include "app/connection.h"
#include "app/message.h"

namespace app {

struct Request {
  Connection* conn;
  Message msg;

  Request(Connection* conn_, const Message& msg_)
      : conn(conn_)
      , msg(msg_) {
  }
}

}  // namespace app

#endif  // APP_REQUEST_H_
