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

inline uint16_t CharToUint16(char c) {
  return static_cast<uint16_t>(static_cast<uint8_t>(c));
}

inline uint32_t CharToUint32(char c) {
  return static_cast<uint32_t>(static_cast<uint8_t>(c));
}

uint16_t BytesToUint16(ByteOrder byte_order, const char bytes[4]) {
  if (byte_order == kLittleEndian) {
    return CharToUint16(bytes[0]) | CharToUint16(bytes[1]) << 8;
  }

  return CharToUint16(bytes[1]) | CharToUint16(bytes[0]) << 8;
}

uint32_t BytesToUint32(ByteOrder byte_order, const char bytes[4]) {
  if (byte_order == kLittleEndian) {
    return CharToUint16(bytes[0]) | CharToUint16(bytes[1]) << 8 |
           CharToUint16(bytes[2]) << 16 | CharToUint16(bytes[3]) << 24;
  }

  return CharToUint32(bytes[3]) | CharToUint32(bytes[2]) << 8 |
         CharToUint32(bytes[1]) << 16 | CharToUint32(bytes[0]) << 24;
}

}  // app
