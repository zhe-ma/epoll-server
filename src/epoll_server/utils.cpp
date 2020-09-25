#include "epoll_server/utils.h"

#include <chrono>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>

#include "epoll_server/logging.h"

namespace epoll_server {

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

int Recv(int fd, char* buf, size_t buf_len) {
  ssize_t n = recv(fd, buf, buf_len, 0);

  // 对端的socket已正常关闭。
  if (n == 0) {
    SPDLOG_TRACE("Remote socket close.");
    return -1;
  }

  if (n > 0) {
    return n;
  }

  // EAGAIN这个操作可能等下重试后可用。它的另一个名字叫做EWOULDAGAIN。
  int err = errno;
  if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
    return 0;
  }

  SPDLOG_WARN("Failed to recv data. Error: {}-{}.", err, strerror(err));
  return -1;
}

int Send(int fd, const char* buf, size_t buf_len, size_t* sended_size) {
  assert(sended_size != nullptr);

  *sended_size = 0;
  while (buf_len > *sended_size) {
    int n = send(fd, buf + *sended_size, buf_len - *sended_size, 0);
    if (n > 0) {
      *sended_size += n;
      continue;
    }

    int err = errno;    
    if (n == -1 && err == EINTR) {
      continue;
    }

    // 发送缓冲区满了，需要等待可写事件才能继续往发送缓冲区里写数据。
    if (n == -1 && (err == EAGAIN || err == EWOULDBLOCK)) {
      return 0;
    }

    SPDLOG_WARN("Send error: {}:{}.", err, strerror(err));
    return -1;
  }

  return 1;
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

std::string Uint32ToBytes(ByteOrder byte_order, uint32_t num) {
  std::string bytes(4, 0);
  if (byte_order == kLittleEndian) {
    bytes[0] = static_cast<uint8_t>(num);
    bytes[1] = static_cast<uint8_t>(num >> 8);
    bytes[2] = static_cast<uint8_t>(num >> 16);
    bytes[3] = static_cast<uint8_t>(num >> 24);
  } else {
    bytes[3] = static_cast<uint8_t>(num);
    bytes[2] = static_cast<uint8_t>(num >> 8);
    bytes[1] = static_cast<uint8_t>(num >> 16);
    bytes[0] = static_cast<uint8_t>(num >> 24);
  }

  return bytes;
}

std::string Uint16ToBytes(ByteOrder byte_order, uint16_t num) {
  std::string bytes(2, 0);
  if (byte_order == kLittleEndian) {
    bytes[0] = static_cast<uint8_t>(num);
    bytes[1] = static_cast<uint8_t>(num >> 8);
  } else {
    bytes[1] = static_cast<uint8_t>(num);
    bytes[0] = static_cast<uint8_t>(num >> 8);
  }

  return bytes;
}

int64_t GetNowTimestamp() {
  using namespace std::chrono;
  auto now = system_clock::now().time_since_epoch();
  return duration_cast<milliseconds>(now).count();
}

}  // namespace epoll_server
