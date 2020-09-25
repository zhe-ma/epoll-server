#include "app/connection.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "app/logging.h"
#include "app/message.h"
#include "app/utils.h"

namespace app {

Connection::Connection(int fd, Type type)
    : fd_(fd)
    , type_(type)
    , epoll_events_(0)
    , timestamp_(0)
    , remote_port_(-1)
    , recv_header_(Message::kHeaderLen, 0)
    , recv_header_len_(0)
    , recv_data_len_(0)
    , sended_len_(0) {
}

Connection::~Connection() {
  Close();
}

void Connection::Close() {
  if (fd_ != -1) {
    close(fd_);
  }

  fd_ = -1;
  type_ = kTypeSocket;
  timestamp_ = 0;
  epoll_events_ = 0;
  remote_ip_.clear();
  remote_port_ = -1;
  recv_header_len_ = 0;
  recv_data_len_ = 0;
  sended_len_ = 0;
}

void Connection::SetSendData(std::string&& send_data, size_t sended_len) {
  send_data_ = std::move(send_data);
  sended_len_ = sended_len;
}

void Connection::UpdateTimestamp() {
  timestamp_ = GetNowTimestamp();
}

int Connection::HandleAccept(struct sockaddr_in* sock_addr) {
  if (type_ != kTypeAcceptor) {
    return -1;
  }

  socklen_t sock_len = 0;
  if (sock_addr != nullptr) {
   sock_len = sizeof(struct sockaddr_in);
   memset(sock_addr, 0, sock_len);
  }

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
  if (type_ != kTypeSocket) {
    return false;
  }

  // Msg Bytes: DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.
  // Receive header.
  if (recv_header_len_ < Message::kHeaderLen) {
    int n = sock::Recv(fd_, &recv_header_[0] + recv_header_len_, Message::kHeaderLen - recv_header_len_);
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
  int n = sock::Recv(fd_, &recv_data_[0] + recv_data_len_, recv_data_.size() - recv_data_len_);
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

bool Connection::HandleWrite() {
  if (type_ != kTypeSocket) {
    return false;
  }

  if (send_data_.empty()) {
    return true;
  }

  size_t buf_size = send_data_.size() - sended_len_;
  int n = sock::Send(fd_, &send_data_[0] + sended_len_, buf_size, &sended_len_);
  // Send data completely.
  if (n > 0) {
    return true;
  }

  // Failed to send data or send data partly.
  return false;
}

void Connection::HandleWakeUp() {
  if (type_ != kTypeWakener) {
    return;
  }

  uint64_t one = 1;
  ssize_t n = read(fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    SPDLOG_ERROR("Failed to HandleWakeUp.");
  }
}

void Connection::SetReadEvent(bool enable) {
  if (enable) {
    epoll_events_ |= (EPOLLIN | EPOLLRDHUP);
  } else {
    epoll_events_ &= ~(EPOLLIN | EPOLLRDHUP);
  }
}

void Connection::SetWriteEvent(bool enable) {
  if (enable) {
    epoll_events_ |= EPOLLOUT;
  } else {
    epoll_events_ &= ~EPOLLOUT;
  }
}

}  // namespace app
