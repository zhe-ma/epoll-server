#include "app/socket.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "app/logging.h"

namespace app {


namespace socket {

// 设置为非阻塞模式.
bool EnableNonBlocking(int fd) {
  int non_blocking = 1;
  ret = ioctl(fd, FIONBIO, &non_blocking);
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
  ret = bind(fd, (const struct sockaddr*)&sock_addr, sizeof(sock_addr));
  if (ret == -1) {
    return false;
  }

  return true;
}

// 打开SO_REUSEADDR选项，防止TIME_WAIT时bind失败的问题.
bool EnableReuseAddr(int fd) {
  int reuse_addr = 1;
  int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse_addr,
                      sizeof(reuse_addr));
  if (ret == -1) {
    return false;
  }

  return true;
}
  
}  // namespace socket

Socket::Socket()
    : listening_socket_fd_(0) {
}

Socket::~Socket() {
  Close();
}

bool Socket::Listen(std::uint16_t port) {
  SPDLOG_TRACK_METHOD;

  SPDLOG_DEBUG("Port: {}.", port);

  // SOCK_STREAM: 表示使用TCP连接.
  listening_socket_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (listening_socket_fd_ == -1) {
    SPDLOG_ERROR("Failed to create socket.");
    return false;
  }

  // 打开SO_REUSEADDR选项，防止TIME_WAIT时bind失败的问题.
  int reuse_addr = 1;
  int ret = setsockopt(listening_socket_fd_, SOL_SOCKET, SO_REUSEADDR,
                       (const void*)&reuse_addr, sizeof(reuse_addr));
  if (ret == -1) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    Close();
    return false;
  }


  // 设置为非阻塞模式.
  int non_blocking = 1;
  ret = ioctl(listening_socket_fd_, FIONBIO, &non_blocking);
  if (ret == -1) {
    SPDLOG_ERROR("Faild to set socket to be non-blocking.");
    Close();
    return false;
  }

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
  ret = bind(listening_socket_fd_, (const struct sockaddr*)&sock_addr, sizeof(sock_addr));
  if (ret == -1) {
    SPDLOG_ERROR("Failed to bind.");
    Close();
    return false;
  }

  // 监听.
  const int kListenBacklog = 511;
  ret = listen(listening_socket_fd_, kListenBacklog);
  if (ret == -1) {
    SPDLOG_ERROR("Failed to listen.");
    Close();
    return false;
  }

  return true;
}

void Socket::Close() {
  SPDLOG_TRACK_METHOD;

  if (listening_socket_fd_ != 0) {
    close(listening_socket_fd_);
  }
}


}  // app
