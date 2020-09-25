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
    , wakener_fd_(-1)
    , connection_pool_(MAX_CONNECTION_POLL_SIZE) {
}

bool Server::Start() {
  SPDLOG_TRACK_METHOD;

  if (!epoller_.Create()) {
    return false;
  }

  // SOCK_STREAM: TCP. Sequenced, reliable, connection-based byte streams.
  // SOCK_CLOEXEC: Atomically set close-on-exec flag for the new descriptor(s).
  acceptor_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (acceptor_fd_ == -1) {
    SPDLOG_ERROR("Failed to create socket.");
    return false;
  }

  acceptor_connection_.reset(new Connection(acceptor_fd_, Connection::kTypeAcceptor));
  acceptor_connection_->SetReadEvent(true);
  if (!epoller_.Add(acceptor_connection_->fd(), acceptor_connection_->epoll_events(),
      static_cast<void*>(acceptor_connection_.get()))) {
    return false;
  }

  wakener_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (wakener_fd_ == -1) {
    SPDLOG_ERROR("Failed to create eventfd instance.");
    return false;
  }

  wakener_connection_.reset(new Connection(wakener_fd_, Connection::kTypeWakener));
  wakener_connection_->SetReadEvent(true);
  if (!epoller_.Add(wakener_connection_->fd(), wakener_connection_->epoll_events(),
      static_cast<void*>(wakener_connection_.get()))) {
    return false;
  }

  // TODO: configurable.
  request_thread_pool_.Start(4, [this](MessagePtr msg) {
    HandleRequest(msg);
  });

  if (!Listen()) {
    return false;
  }

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
  // Enable SO_REUSEADDR option to avoid that TIME_WAIT prevents the server restarting.
  if (!sock::SetReuseAddr(acceptor_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    return false;
  }

  // Set the acceptor to be non-blocking to avoid calling accept() to block too much time.
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

    // The connection may be closed by preivous event.
    if (conn->fd() == -1) {
      SPDLOG_DEBUG("Expired event.");
      continue;
    }

    // The socket is disconected if receive EPOLLERR or EPOLLRDHUP.
    if (event.events & (EPOLLIN | EPOLLERR | EPOLLRDHUP)) {
      if (conn->type() == Connection::kTypeAcceptor) {
        HandleAccpet(conn);
      } else if (conn->type() == Connection::kTypeSocket) {
        HandleRead(conn);
      } else if (conn->type() == Connection::kTypeWakener) {
        conn->HandleWakeUp();
      }
      continue;
    }

    if (event.events & EPOLLOUT) {
      // The EPOLLOUT event will be triggered constantly if the socket is writable.
      // So if the data is sened completely, the EPOLLOUT event should be removed from epoll.
      if (conn->HandleWrite()) {
        conn->SetWriteEvent(false);
        epoller_.Modify(conn->fd(), conn->epoll_events(), static_cast<void*>(conn));
      }
    }
  }

  HandlePendingResponses();

  return true;
}

// In I/O thread.
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
  }
}

// In I/O thread.
void Server::HandleRead(Connection* conn) {
  MessagePtr request;
  if (!conn->HandleRead(&request)) {
    connection_pool_.Release(conn);
    return;
  }

  if (!request) {
    return;
  }

  request_thread_pool_.Add(std::move(request));
}

// In I/O thread.
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

    if (response->IsExpired()) {
      SPDLOG_DEBUG("Expired reponse.");
      continue;
    }

    Connection* conn = response->conn();
    std::string buf = std::move(response->Pack());
    size_t sended_size = 0;

    int n = sock::Send(conn->fd(), &buf[0], buf.size(), &sended_size);
    // Send error.
    if (n == -1) {
      SPDLOG_ERROR("Failed to send data.");
      continue;
    }

    // System write buffer is full. It should wait the writable event.
    if (n == 0) {
      conn->SetSendData(std::move(buf), sended_size);
      conn->SetWriteEvent(true);
      epoller_.Modify(conn->fd(), conn->epoll_events(), static_cast<void*>(conn));
    }
  }
}

// Use thread pool to handle requests.
void Server::HandleRequest(MessagePtr request) {
  if (!request && !request->Valid()) {
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

  // Wake up epoll_wait to handle pending responses.
  WakeUp();
}

void Server::WakeUp() {
  uint64_t one = 1;
  ssize_t n = write(wakener_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    SPDLOG_ERROR("Failed to wakeup.");
  }
}

}  // namespace app
