#include "app/server.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/errno.h>

#include "app/crc32.h"
#include "app/logging.h"
#include "app/utils.h"
#include "app/connection.h"
#include "app/message.h"

namespace app {

// TODO: Configubale
const size_t MAX_CONNECTION_POLL_SIZE = 1024;

Server::Server(unsigned short port)
    : port_(port)
    , listen_fd_(-1)
    , event_fd_(-1)
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

  if (!epoller_.Create()) {
    close(listen_fd_);
    return false;
  }

  Connection* conn = connection_pool_.Get();
  conn->set_fd(listen_fd_);
  conn->set_type(Connection::kTypeAcceptor);

  if (!UpdateEpollEvent(listen_fd_, conn, EPOLL_CTL_ADD, true, false)) {
    connection_pool_.Release(conn);
    return false;
  }

  event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (event_fd_ == -1) {
    SPDLOG_ERROR("Failed to create eventfd instance.");
    close(listen_fd_);
    close(event_fd_);
    return false;
  }

  // TODO: configurable.
  request_thread_pool_.Start(4, [this](MessagePtr msg) {
    HandleRequest(msg);
  });

  // TODO: delete conn.
  for (;;) {
    if (!PollOnce()) {
      return false;
    }
  }

  request_thread_pool_.StopAndWait();

  return true;
}

void Server::AddRouter(uint16_t msg_code, RouterPtr router) {
  routers_[msg_code] = router;
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

  // if(epoll_ctl(epoll_fd_, event_type, socket_fd, &ev) == -1) {
  //   SPDLOG_ERROR("Failed to update epoll events. Error:{}-{}。", errno, strerror(errno));
  //   return false;
  // }

  return true;
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

bool Server::PollOnce() {
  int n = epoller_.Poll();
  if (n == -1 ) {
    return false;
  }

  for (int i = 0; i < n; ++i) {
    auto event = epoller_.GetEvent(i);
    Connection* conn = static_cast<Connection*>(event.data.ptr);
    if (conn == nullptr) {
      SPDLOG_WARN("Unexpected epoll event.");
      continue;
    }

    // 如果epoll_wait拿到多个事件，第一个事件由于业务需要将conn->socket_fd置为-1。
    // 而第二个事件还是这个连接，那个这个连接就属于过期的事件。
    if (conn->fd() == -1) {
      SPDLOG_DEBUG("Expired event.");
      continue;
    }

    int events = event.events;
    // 如果收到EPOLLERR或者EPOLLRDHUP，socket断掉，则在HanldeRead和HandleWrite中进行处理。
    if (events & (EPOLLERR | EPOLLRDHUP) ) {
      events |= EPOLLIN | EPOLLOUT; 
    }

    if (events & EPOLLIN) {
      if (conn->type() == Connection::kTypeAcceptor) {
        HandleAccpet(conn);
      } else if (conn->type() == Connection::kTypeSocket) {
        HandleRead(conn);
      } else if (conn->type() == Connection::kTypeEventFd) {
        conn->HandleWakeUp();
      }

    } else if (events & EPOLLOUT) {
      if (conn->HandleWrite()) {
        // UpdateEpollEvent();
      }
    }
  }

  HandlePendingResponses();

  return true;
}

void Server::HandleAccpet(Connection* conn) {
  if (conn == nullptr) {
    return;
  }

  struct sockaddr_in sock_addr;
  int fd = conn->HandleAccept(&sock_addr);
  if (fd == -1) {
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

  Connection* new_conn = connection_pool_.Get();
  if (new_conn == nullptr) {
    SPDLOG_WARN("Connection pool is empty. Remote addr: {}:{}.", remote_ip, remote_port);
    return;
  }

  new_conn->set_fd(fd);
  new_conn->set_remote_ip(remote_ip);
  new_conn->set_remote_port(remote_port);

  if (!UpdateEpollEvent(fd, new_conn, EPOLL_CTL_ADD, true, false)) {
    SPDLOG_ERROR("Failed to update epoll event. Remote addr: {}:{}.", remote_ip, remote_port);
    connection_pool_.Release(new_conn);
    return;
  }
}

void Server::HandleRead(Connection* conn) {
  MessagePtr msg;
  if (!conn->HandleRead(&msg)) {
    connection_pool_.Release(conn);
    return;
  }

  if (!msg) {
    return;
  }
  
  SPDLOG_TRACE("Recv:{},{},{},{}", msg->data_len, msg->code, msg->crc32, msg->data);

  request_thread_pool_.Add(std::move(msg));
}

void Server::HandleRequest(MessagePtr request) {
  if (!request) {
    return;
  }

  SPDLOG_TRACE("Handle request: {}", request->data);

  if (!request->Valid()) {
    return;
  }

  auto it = routers_.find(request->code);
  if (it == routers_.end()) {
    SPDLOG_WARN("No msg router. Msg code:{}.", request->code);
    return;
  }

  RouterPtr router = it->second;
  if (!router) {
    SPDLOG_WARN("No msg router. Msg code:{}.", request->code);
    return;
  }

  std::string response_data = router->HandleRequest(request);
  MessagePtr response = std::make_shared<Message>(request->conn(), request->code, std::move(response_data));

  {
    std::lock_guard<std::mutex> lock(pending_response_mutex_);
    pending_responses_.push_back(std::move(response));
  }

  WakeUp();
}

void Server::HandlePendingResponses() {
  std::vector<MessagePtr> responses;
  {
    std::lock_guard<std::mutex> lock(pending_response_mutex_);
    responses.swap(pending_responses_);
  }

  for (auto response : responses) {
    if (!response) {
      continue;
    }

    SPDLOG_TRACE("Handle Response: {}", response->data);

    // // TODO: Check expiration.

    Connection* conn = response->conn();
    if (conn == nullptr) {
      continue;
    }

    std::string buf = std::move(response->Pack());
    size_t sended_size = 0;
    int n = sock::Send(conn->fd(), &buf[0], buf.size(), &sended_size);
    if (n == -1) {
      SPDLOG_ERROR("Failed to send data.");
    } else if (n == 0) {
       // 发送缓冲区满了，需要等待可写事件才能继续往发送缓冲区里写数据。
      // UpdateEpollEvent();
      conn->SetSendData(std::move(buf), sended_size);
    }
  }
}

void Server::WakeUp() {
  uint64_t one = 1;
  ssize_t n = write(event_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    SPDLOG_ERROR("Failed to wakeup.");
  }
}

}  // namespace app
