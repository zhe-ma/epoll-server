#include "app/server.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/errno.h>

#include "app/logging.h"
#include "app/utils.h"
#include "app/connection.h"

namespace app {

// TODO: Configubale
const size_t MAX_EPOLL_EVENTS = 512;
const size_t MAX_CONNECTION_POLL_SIZE = 1024;

Server::Server(unsigned short port)
    : port_(port)
    , listen_fd_(-1)
    , epoll_fd_(-1)
    , epoll_events_(MAX_EPOLL_EVENTS)
    , connection_pool_(MAX_CONNECTION_POLL_SIZE) {
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

  Connection* conn = connection_pool_.Get();
  conn->set_server(this);
  conn->set_socket_fd(listen_fd_);
  conn->set_is_listen_socket(true);

  if (!UpdateEpollEvent(listen_fd_, conn, EPOLL_CTL_ADD, true, false)) {
    connection_pool_.Release(conn);
    return false;
  }

  // TODO: delete conn.
  for (;;) {
    if (!PollOnce(-1)) {
      return false;
    }
  }

  return true;
}

// EPOLLIN: socket可以读, 对端SOCKET正常关闭, 客户端三次握手连接。
// EPOLLOUT: socket可以写。
// EPOLLPRI: socket紧急的数据(带外数据到来)可读。
// EPOLLERR: 客户端socket发生错误, 断电之类的断开，server端并不会在这种情况收到该事件。
// EPOLLRDHUP: 读关闭，2.6.17之后版本内核，对端连接断开(close()，kill，ctrl+c)触发的epoll事件会包含EPOLLIN|EPOLLRDHUP。
// EPOLLHUP: 读写都关闭
// EPOLLET: 将EPOLL设为边缘触发(Edge Triggered)模式。
bool Server::UpdateEpollEvent(int socket_fd, Connection* conn, int event_type, bool read, bool write) {
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
 
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

Connection* Server::GetConnection() {
  return connection_pool_.Get();
}

void Server::ReleaseConnection(Connection* conn) {
  connection_pool_.Release(conn);
}

bool Server::Listen() {
  SPDLOG_TRACK_METHOD;

  // 打开SO_REUSEADDR选项，防止TIME_WAIT时bind失败的问题.
  if (!sock::SetReuseAddr(listen_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    return false;
  }

  // 设置为非阻塞模式，防止accept时阻塞太久。
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

bool Server::PollOnce(int waiting_ms) {
  // 等待事件，最多返回MAX_EPOLL_EVENTS个。
  // waiting_ms = -1时，一直阻塞直到收到事件。
  // waiting_ms > 0 时，阻塞时间到达或者阻塞时收到事件都会返回。
  // 返回值n，收到的事件数。
  int n = epoll_wait(epoll_fd_, &epoll_events_[0], epoll_events_.size(), waiting_ms);

  // n = -1, 有错误产生。
  if (n == -1 ) {
    SPDLOG_ERROR("Failed to epoll wait. Error: {}-{}.", errno, strerror(errno));
    // EINTR: Interrupted system call, epoll_wait被更高级的系统调用打断, 错误可以忽略。
    // 其他错误则提出。
    return errno == EINTR;
  }

  // n = 0, 超时。
  if (n == 0) {
    // waiting_ms = -1时，一直阻塞等待有事件才返回，如果此时返回n=0, 则存在问题。
    if (waiting_ms == -1) {
      SPDLOG_ERROR("No events receieved when epoll_wait timeout.");
      return false;
    }

    return true;
  }

  // n > 0
  for (int i = 0; i < n; ++i) {
    Connection* conn = static_cast<Connection*>(epoll_events_[i].data.ptr);
    if (conn == nullptr) {
      SPDLOG_WARN("Unexpected epoll event.");
      continue;
    }

    // 如果epoll_wait拿到多个事件，第一个事件由于业务需要将conn->socket_fd置为-1。
    // 而第二个事件还是这个连接，那个这个连接就属于过期的事件。
    if (conn->socket_fd() == -1) {
      SPDLOG_DEBUG("Expired event.");
      continue;
    }

    int events = epoll_events_[i].events;

    // 如果收到EPOLLERR或者EPOLLRDHUP，socket断掉，则在HanldeRead和HandleWrite中进行处理。
    if (events & (EPOLLERR | EPOLLRDHUP) ) {
      events |= EPOLLIN | EPOLLOUT; 
    }

    if (events & EPOLLIN) {
      conn->HandleRead();
      continue;
    }

    if (events & EPOLLOUT) {
      conn->HandleWrite();
      continue;
    }
  }

  return true;
}

}  // namespace app
