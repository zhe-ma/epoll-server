#ifndef APP_SERVER_H_
#define APP_SERVER_H_

#include <string>

namespace app {

class Server {
public:
  Server() = default;
  ~Server() = default;

  bool Start(std::size_t worker_count);
};

}  // namespace app

#endif  // APP_SERVER_H_
