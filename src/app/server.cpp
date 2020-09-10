#include "app/server.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include "app/logging.h"
#include "app/utils.h"
#include "app/connection.h"

namespace app {

Server::Server(unsigned short port)
    : port_(port)
    , listen_fd_(-1)
    , epoll_fd_(-1) {
}

bool Server::Start() {
  // SOCK_STREAM: TCP. Sequenced, reliable, connection-based byte streams.
  // SOCK_CLOEXEC: Atomically set close-on-exec flag for the new descriptor(s).
  listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (listen_fd_ == -1) {
    SPDLOG_ERROR("Failed to create socket.");
    return false;
  }

  if (!Listen()) {
    close(listen_fd_);
    return false;
  }

  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ == -1) {
    SPDLOG_ERROR("Failed to create epoll instance.");
    close(listen_fd_);
    return false;
  }

  Connection* conn = new Connection(listen_fd_);
  if (!UpdateEpollEvent(listen_fd_, conn, EPOLL_CTL_ADD, true, false)) {
    return false;
  }

  return true;
}

bool Server::UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));

  // EPOLLIN: socket可以读, 对端SOCKET正常关闭, 客户端三次握手连接。
  // EPOLLOUT: socket可以写。
  // EPOLLPRI: socket紧急的数据(带外数据到来)可读。
  // EPOLLERR: 客户端socket发生错误；
  // EPOLLHUP: 2.6.17之后版本内核，对端连接断开(close()，kill，ctrl+c)触发的epoll事件会包含EPOLLIN|EPOLLRDHUP。
  // EPOLLET: 将EPOLL设为边缘触发(Edge Triggered)模式。
  
  if (read) {
   ev.events = EPOLLIN|EPOLLRDHUP;
  } else if(write) {

  }
  
  ev.data.ptr = conn;

  if(epoll_ctl(epoll_fd_, event_type, socket_fd,&ev) == -1) {
    SPDLOG_ERROR("Failed to update epoll events. Error:{}-{}。", errno, strerror(errno));
    return false;
  }

  return true;
}

bool Server::Listen() {
  SPDLOG_TRACK_METHOD;

  // 打开SO_REUSEADDR选项，防止TIME_WAIT时bind失败的问题.
  if (!sock::SetReuseAddr(listen_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    return false;
  }

  // 为什么要设置为非阻塞模式？？？？？？
  if (!sock::SetNonBlocking(listen_fd_)) {
    SPDLOG_ERROR("Faild to set socket to be non-blocking.");
    return false;
  }

  if (!sock::Bind(listen_fd_, port_)) {
    SPDLOG_ERROR("Failed to bind port: {}.", port_);
    return false;
  }

  const int kListenBacklog = 511;
  if (listen(listen_fd_, kListenBacklog) == -1) {
    SPDLOG_ERROR("Failed to listen.");
    return false;
  }

  return true;
}

}  // namespace app
