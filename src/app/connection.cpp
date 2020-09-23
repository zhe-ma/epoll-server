#include "app/connection.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "app/logging.h"
#include "app/message.h"
#include "app/utils.h"

namespace app {

Connection::Connection()
    : fd_(-1)
    , timestamp_(0)
    , is_listen_socket_(false)
    , remote_port_(-1)
    , recv_header_(Message::kHeaderLen, 0)
    , recv_header_len_(0)
    , recv_data_len_(0) {
}

Connection::~Connection() {
  Close();
}

void Connection::Close() {
  if (fd_ != -1) {
    close(fd_);
  }

  timestamp_.store(0, std::memory_order_relaxed);

  fd_ = -1;
  is_listen_socket_ = false;
  remote_ip_.clear();
  remote_port_ = -1;
  recv_header_len_ = 0;
  recv_data_len_ = 0;
}

void Connection::UpdateTimestamp() {
  auto now = GetNowTimestamp();
  timestamp_.store(now, std::memory_order_relaxed);
}

int64_t Connection::GetTimestamp() const {
  int64_t a = timestamp_.load(std::memory_order_relaxed);
  return a;
}

int Connection::HandleAccept(struct sockaddr_in* sock_addr) {
  if (fd_ <= 0) {
    SPDLOG_CRITICAL("The listening socket fd is reset to {}.", fd_);
    return -1;
  }

  socklen_t sock_len = 0;
  if (sock_addr != nullptr) {
   sock_len = sizeof(struct sockaddr_in);
   memset(sock_addr, 0, sock_len);
  }

  // 将listen socket设置为非阻塞，防止该函数阻塞太久。
  int fd = accept(fd_, (struct sockaddr*)sock_addr, &sock_len);
  if (fd != -1) {
    return fd;
  }

  // EAGAIN是提示再试一次。这个错误经常出现在当应用程序进行一些非阻塞操作。
  // 如果read操作而没有数据可读，此时程序不会阻塞起来等待数据准备就绪返回，
  // read函数会返回一个错误EAGAIN，提示现在没有数据可读请稍后再试。
  int err = errno;
  if (err != EAGAIN && err != EINTR) {
    SPDLOG_WARN("Failed to accept socket. Error: {}-{}.", err, strerror(err));
  }

  return -1;
}

bool Connection::HandleRead(MessagePtr* msg) {
  // Msg Bytes: DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.

  // Receive header.
  if (recv_header_len_ < Message::kHeaderLen) {
    int n = Recv(&recv_header_[0] + recv_header_len_, Message::kHeaderLen - recv_header_len_);
    if (n < 0) {
      return false;
    } else if (n == 0) {
      return true;
    }

    recv_header_len_ += n;
    // Header没接收收完整则返回，继续接收数据。
    if (recv_header_len_ != Message::kHeaderLen) {
      return true;
    }

    // Header接收完整，计算出数据长度。
    std::uint16_t data_len = BytesToUint16(kLittleEndian, &recv_header_[0]);
    // 数据长度大于最大包数据长度，认为是恶意包，丢弃该数据，重新接收数据。
    if (data_len > 3000) {  // TODO: configuable.
      recv_header_len_ = 0;
      return true;
    }

    recv_data_.resize(data_len);
    return true;
  }

  // Recveive data.
  int n = Recv(&recv_data_[0] + recv_data_len_, recv_data_.size() - recv_data_len_);
  if (n <= 0) {
    return false;
  } else if (n == 0) {
    return true;
  }

  recv_data_len_ += n;
  // 数据接收不完整则返回，继续接收数据。
  if (recv_data_len_ != recv_data_.size()) {
    return true;
  }

  // 数据接收完整返回Message。
  if (msg != nullptr) {
    msg->reset(new Message);
    (*msg)->Unpack(this, &recv_header_[0], std::move(recv_data_));
  }

  recv_header_len_ = 0;
  recv_data_len_ = 0;

  return true;
}

void Connection::HandleWrite() {
}

int Connection::Recv(char* buf, size_t buf_len) {
  ssize_t n = recv(fd_, buf, buf_len, 0);

  // 对端的socket已正常关闭。
  if (n == 0) {
    SPDLOG_TRACE("Remote socket close. Remote addr: {}:{}.", remote_ip_, remote_port_);
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

  SPDLOG_WARN("Failed to recv data. RemoteAddr:{}. Error: {}-{}.", remote_ip_, err, strerror(err));
  return -1;
}

}  // namespace app
