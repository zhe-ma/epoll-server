#ifndef APP_SERVER_H_
#define APP_SERVER_H_

namespace app {

class Server {
public:
  explicit Server(unsigned short port);

  ~Server() = default;

  bool Start();

private:
  bool Listen();

private:
  unsigned short port_;

  int listen_fd_;

};

}  // namespace app

#endif  // APP_SERVER_H_
