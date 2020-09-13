#include "app/utils.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#include "app/logging.h"

namespace app {

namespace sock {

bool SetNonBlocking(int fd) {
  int non_blocking = 1;
  int ret = ioctl(fd, FIONBIO, &non_blocking);
  if (ret == -1) {
    return false;
  }

  return true;
}

bool SetClosexc(int fd) {
  int ret = fcntl(fd, F_GETFD);
  ret = fcntl(fd, F_SETFD, ret | FD_CLOEXEC);
  if (ret == -1) {
    return false;
  }

  return true;
}

bool Bind(int fd, unsigned short port) {
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));

  // AF_INET: IPV4协议族.
  sock_addr.sin_family = AF_INET;
  // htonl: 将主机数转换成无符号长整型的网络字节顺序.
  // INADDR_ANY: 表示一个服务器上所有的网卡的所有个本地IP地址.
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // htons: 将主机数转换成无符号短整型的网络字节顺序.
  sock_addr.sin_port = htons(port);

  // 绑定地址.
  int ret = bind(fd, (const struct sockaddr*)&sock_addr, sizeof(sock_addr));
  if (ret == -1) {
    return false;
  }

  return true;
}

bool SetReuseAddr(int fd) {
  int reuse_addr = 1;
  int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse_addr,
                      sizeof(reuse_addr));
  if (ret == -1) {
    return false;
  }

  return true;
}
  
}  // namespace sock

}  // app
