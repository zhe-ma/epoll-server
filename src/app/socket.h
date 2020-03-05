#ifndef APP_SOCKET_H_
#define APP_SOCKET_H_

#include <string>

namespace app {

// -----------------------------------------------------------------------------

class Socket {
public:
  Socket();
  virtual ~Socket();

  bool Listen(std::uint16_t port);

  void Close();

private:
  int listening_socket_fd_;
};

}  // namespace app

#endif  // APP_SOCKET_H_
