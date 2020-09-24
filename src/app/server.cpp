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
    , acceptor_fd_(-1)
    , event_fd_(-1)
    , connection_pool_(MAX_CONNECTION_POLL_SIZE) {
}

bool Server::Start() {
  // SOCK_STREAM: TCP. Sequenced, reliable, connection-based byte streams.
  // SOCK_CLOEXEC: Atomically set close-on-exec flag for the new descriptor(s).
  acceptor_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (acceptor_fd_ == -1) {
    SPDLOG_ERROR("Failed to create socket.");
    return false;
  }

  if (!Listen()) {
    close(acceptor_fd_);
    return false;
  }

  if (!epoller_.Create()) {
    close(acceptor_fd_);
    return false;
  }

  Connection* conn = connection_pool_.Get();
  conn->set_fd(acceptor_fd_);
  conn->set_type(Connection::kTypeAcceptor);
  conn->SetReadEvent(true);

  if (!epoller_.Add(conn->fd(), conn->epoll_events(), static_cast<void*>(conn))) {
    connection_pool_.Release(conn);
    return false;
  }

  event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (event_fd_ == -1) {
    SPDLOG_ERROR("Failed to create eventfd instance.");
    close(acceptor_fd_);
    connection_pool_.Release(conn);
    return false;
  }

  conn = connection_pool_.Get();
  conn->set_fd(event_fd_);
  conn->set_type(Connection::kTypeEventFd);
  conn->SetReadEvent(true);

  if (!epoller_.Add(conn->fd(), conn->epoll_events(), static_cast<void*>(conn))) {
    connection_pool_.Release(conn);
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

bool Server::Listen() {
  SPDLOG_TRACK_METHOD;

  // 打开SO_REUSEADDR选项，防止TIME_WAIT时bind失败的问题.
  if (!sock::SetReuseAddr(acceptor_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    return false;
  }

  // 设置为非阻塞模式，防止accept时阻塞太久。
  if (!sock::SetNonBlocking(acceptor_fd_)) {
    SPDLOG_ERROR("Faild to set socket to be non-blocking.");
    return false;
  }

  if (!sock::Bind(acceptor_fd_, port_)) {
    SPDLOG_ERROR("Failed to bind port: {}.", port_);
    return false;
  }

  const int kListenBacklog = 511;
  if (listen(acceptor_fd_, kListenBacklog) == -1) {
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
        conn->SetWriteEvent(false);
        epoller_.Modify(conn->fd(), conn->epoll_events(), static_cast<void*>(conn));
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
  new_conn->SetReadEvent(true);

  if (!epoller_.Add(new_conn->fd(), new_conn->epoll_events(), static_cast<void*>(new_conn))) {
    SPDLOG_ERROR("Failed to update epoll event. Remote addr: {}:{}.", remote_ip, remote_port);
    connection_pool_.Release(conn);
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
       // System write buffer is full. It should wait the writable event.
      conn->SetSendData(std::move(buf), sended_size);
      conn->SetWriteEvent(true);
      epoller_.Modify(conn->fd(), conn->epoll_events(), static_cast<void*>(conn));
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
