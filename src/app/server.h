#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include "app/socket.h"

namespace app {

class Server {
public:
  Server() = default;
  ~Server() = default;

  bool Start();

private:
  Socket socket_;
};

}  // namespace app

#endif  // APP_SERVER_H_
