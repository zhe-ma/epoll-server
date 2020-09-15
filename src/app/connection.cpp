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
    : socket_fd_(-1)
    , server_(nullptr)
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
  if (socket_fd_ != -1) {
    close(socket_fd_);
  }

  socket_fd_ = -1;
  server_ = nullptr;
  is_listen_socket_ = false;
  remote_ip_.clear();
  remote_port_ = -1;
  recv_header_len_ = 0;
  recv_data_len_ = 0;
}

void Connection::HandleRead() {
  if (is_listen_socket_) {
    DoAccept();
    return;
  }

  DoRead();
}

void Connection::HandleWrite() {
}

void Connection::DoRead() {
  // Msg Bytes: DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.

  // Receive header.
  if (recv_header_len_ < Message::kHeaderLen) {
    int n = Recv(&recv_header_[0] + recv_header_len_, Message::kHeaderLen - recv_header_len_);
    if (n == -1) {
      return;
    }

    recv_header_len_ += n;
    if (recv_header_len_ != Message::kHeaderLen) {
      return;
    }

    std::uint16_t data_len = BytesToUint16(kLittleEndian, &recv_header_[0]);
    // 数据长度大于最大包数据长度，认为是恶意包，丢弃该数据，重新接收数据。
    if (data_len > 3000) {  // TODO: configuable.
      recv_header_len_ = 0;
      return;
    }

    recv_data_.resize(data_len);
    return;
  }

  // Recveive data.
  int n = Recv(&recv_data_[0] + recv_data_len_, recv_data_.size() - recv_data_len_);
  if (n == -1) {
    return;
  }
  recv_data_len_ += n;

  if (recv_data_len_ == recv_data_.size()) {
    Message msg(&recv_header_[0], std::move(recv_data_));
    SPDLOG_TRACE("Recv:{},{},{},{}", msg.data_len, msg.code,msg.crc32, msg.data);
    recv_header_len_ = 0;
    recv_data_len_ = 0;
  }
}

void Connection::DoAccept() {
  if (socket_fd_ <= 0) {
    SPDLOG_CRITICAL("The listening socket fd is reset to {}.", socket_fd_);
    return;
  }
  
  struct sockaddr_in sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  socklen_t sock_len = sizeof(sock_addr);

  // 将listen socket设置为非阻塞，防止该函数阻塞太久。
  int fd = accept(socket_fd_, (struct sockaddr*)&sock_addr, &sock_len);
  if (fd == -1) {
    // EAGAIN是提示再试一次。这个错误经常出现在当应用程序进行一些非阻塞操作。
    // 如果read操作而没有数据可读，此时程序不会阻塞起来等待数据准备就绪返回，
    // read函数会返回一个错误EAGAIN，提示现在没有数据可读请稍后再试。
    int err = errno;
    if (err == EAGAIN || err == EINTR) {
      return;
    }

    SPDLOG_WARN("Failed to accept socket. Error: {}-{}.", err, strerror(err));
    return;
  }

  char remote_ip[30] = { 0 };
  inet_ntop(AF_INET,&sock_addr.sin_addr, remote_ip, sizeof(remote_ip));
  unsigned short remote_port = ntohs(sock_addr.sin_port);

  SPDLOG_TRACE("Accept socket. Remote addr: {}:{}.", remote_ip, remote_port);

  if (!sock::SetNonBlocking(fd)) {
    SPDLOG_WARN("Failed to SetNonBlocking. Remote addr: {}:{}.", remote_ip, remote_port);
    close(fd);
    return;
  }

  if (!sock::SetClosexc(fd)) {
    SPDLOG_WARN("Failed to SetClosexc. Remote addr: {}:{}.", remote_ip, remote_port);
    close(fd);
    return;
  }

  Connection* conn = server_->GetConnection();
  if (conn == nullptr) {
    SPDLOG_WARN("Connection pool is empty. Remote addr: {}:{}.", remote_ip, remote_port);
    return;
  }

  conn->set_socket_fd(fd);
  conn->set_server(server_);
  conn->set_remote_ip(remote_ip);
  conn->set_remote_port(remote_port);

  if (!server_->UpdateEpollEvent(fd, conn, EPOLL_CTL_ADD, true, false)) {
    SPDLOG_ERROR("Failed to update epoll event. Remote addr: {}:{}.", remote_ip, remote_port);
    server_->ReleaseConnection(conn);
    return;
  }
}

int Connection::Recv(char* buf, size_t buf_len) {
  ssize_t n = recv(socket_fd_, buf, buf_len, 0);

  // 对端的socket已正常关闭。
  if (n == 0) {
    SPDLOG_TRACE("Remote socket close. Remote addr: {}:{}.", remote_ip_, remote_port_);
    server_->ReleaseConnection(this);
    return -1;
  }

  if (n > 0) {
    return n;
  }

  // EAGAIN这个操作可能等下重试后可用。它的另一个名字叫做EWOULDAGAIN。
  int err = errno;
  if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
    return -1;
  }

  SPDLOG_WARN("Failed to recv data. RemoteAddr:{}. Error: {}-{}.", remote_ip_, err, strerror(err));

  server_->ReleaseConnection(this);
  return -1;
}

}  // namespace app
