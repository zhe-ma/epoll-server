#include "app/server.h"

#include <iostream>

#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace app {

void StartServer() {
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;  // IPV4
  addr.sin_port = htons(9004);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  bind(socket_fd, (sockaddr*)&addr, sizeof(addr));
  listen(socket_fd, 32);

  int conn_fd = -1;
  const char* kHello = "Hello!";
  for (size_t i = 0; i < 1; ++i) {
    conn_fd = accept(socket_fd, (sockaddr*)nullptr, nullptr);

    char buf[1000 + 1] = {0};
    read(conn_fd, buf, 1000);
    std::cout << buf <<std::endl;
    
    write(conn_fd, kHello, sizeof(kHello));
    close(conn_fd);
  }

  close(socket_fd);
}

}  // namespace app
