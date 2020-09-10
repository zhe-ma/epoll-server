#ifndef APP_CONNECTION_H_
#define APP_CONNECTION_H_

namespace app {

class Connection {
public:
  Connection(int socket_fd);

private:
  int fd_;
};

}  // namespace app

#endif  // APP_CONNECTION_H_
