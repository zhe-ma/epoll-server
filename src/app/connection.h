#ifndef APP_CONNECTION_H_
#define APP_CONNECTION_H_

namespace app {

class Connection {
public:
  Connection(int socket_fd);

  int socket_fd() const {
    return socket_fd_;
  }

  void HandleRead();
  void HandleWrite();

private:
  int socket_fd_;
};

}  // namespace app

#endif  // APP_CONNECTION_H_
