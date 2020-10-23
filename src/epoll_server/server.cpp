#include "epoll_server/server.h"

#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/errno.h>

#include "epoll_server/crc32.h"
#include "epoll_server/config.h"
#include "epoll_server/logging.h"
#include "epoll_server/utils.h"
#include "epoll_server/connection.h"
#include "epoll_server/message.h"
#include "epoll_server/process.h"

namespace epoll_server {

Server::Server()
    : acceptor_fd_(-1)
    , wakener_fd_(-1)
    , time_wheel_scheduler_(50) {
}

bool Server::Init(int argc, char** argv, const std::string& config_path) {
  g_argc = argc;
  g_argv = argv;

  CONFIG.Load(config_path);

  InitLogging(CONFIG.log_filename, CONFIG.log_file_level, CONFIG.log_rotate_size,
              CONFIG.log_rotate_count, CONFIG.log_console_level);
  SPDLOG_DEBUG("==========================================================");

  InitTimeWheelScheduler();

  if (!InitAcceptor()) {
    return false;
  }

  return true;
}

void Server::Start() {
  if (CONFIG.deamon_mode) {
    if (CreateDaemonProcess() > 0) {
      exit(0);
    }
  }

  if (CONFIG.master_worker_mode) {
    StartMasterAndWorkers();
    return;
  }

  StartServer();
}

void Server::AddRouter(uint16_t msg_code, RouterPtr router) {
  routers_[msg_code] = router;
}

uint32_t Server::CreateTimerAt(int64_t when_ms, const TimerTask& task) {
  return time_wheel_scheduler_.CreateTimerAt(when_ms, task);
}

uint32_t Server::CreateTimerAfter(int64_t delay_ms, const TimerTask& task) {
  return time_wheel_scheduler_.CreateTimerAfter(delay_ms, task);
}

uint32_t Server::CreateTimerEvery(int64_t interval_ms, const TimerTask& task) {
  return time_wheel_scheduler_.CreateTimerEvery(interval_ms, task);
}

void Server::CancelTimer(uint32_t timer_id) {
  time_wheel_scheduler_.CancelTimer(timer_id);
}

bool Server::StartServer() {
  SPDLOG_TRACK_METHOD;

  connection_pool_.reset(new ConnectionPool(CONFIG.connection_pool_size));

  if (!epoller_.Create()) {
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

  request_thread_pool_.Start(CONFIG.thread_pool_size, [this](MessagePtr msg) {
    HandleRequest(msg);
  });

  time_wheel_scheduler_.Start([this](TimerPtr timer) {
    HandleTimeWheelScheduler(timer);
  });

  for (;;) {
    if (!PollOnce()) {
      return false;
    }
  }

  request_thread_pool_.StopAndWait();
  time_wheel_scheduler_.Stop();

  return true;
}

bool Server::StartMasterAndWorkers() {
  SPDLOG_TRACK_METHOD;

  BackupEnviron();

  SetProcessTitle(CONFIG.master_title);

  if (!InitSignals()) {
    return false;
  }

  SPDLOG_DEBUG("Init signals sucessfully.");

  // Block signals before call fork().
  BlockMasterProcessSignals();

  StartWorkers();
  
  sigset_t set;
  sigemptyset(&set);

  // The handling of master process.
  for (;;) {
    // sigsuspend的作用：
    // 1. 根据给定的参数设置新的mask并阻塞当前进程。
    // 2. 如果收到信号，则恢复原先的信号屏蔽。
    // 3. 调用该信号对应的信号处理函数。
    // 4. 信号处理函数返回后，sigsuspend返回，使程序流程继续往下走。
    sigsuspend(&set);  // 阻塞在这里，等待一个信号，此时进程是挂起的，不占用cpu时间，只有收到信号才会被唤醒。
    SPDLOG_DEBUG("I'm master: {}", getpid());
  }

  return true;
}

void Server::StartWorkers() {
  SPDLOG_TRACK_METHOD;

  for (size_t i = 0; i < CONFIG.process_worker_count; ++i) {
    pid_t pid = fork();
    if (pid == -1) {
      SPDLOG_ERROR("Failed to fork worker process.");
      continue;
    }

    if (pid > 0) {
      continue;
    }

    // pid == 0: Child process will execute the following code.

    SPDLOG_DEBUG("Fork a new child process. PID: {}.", getpid());

    // Unmask all signals masked by master process.
    sigset_t set;
    sigemptyset(&set);
    int ret = sigprocmask(SIG_SETMASK, &set, nullptr);
    SPDLOG_DEBUG("Unmask all signals masked by master process: {}.", ret);

    SetProcessTitle(CONFIG.worker_title);
    if (!StartServer()) {
      SPDLOG_DEBUG("Child process failed to start server. PID: {}.", getpid());
    }
  }
}

bool Server::InitAcceptor() {
  // SOCK_STREAM: TCP. Sequenced, reliable, connection-based byte streams.
  // SOCK_CLOEXEC: Atomically set close-on-exec flag for the new descriptor(s).
  acceptor_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (acceptor_fd_ == -1) {
    SPDLOG_ERROR("Failed to create socket.");
    return false;
  }

  // Enable SO_REUSEADDR option to avoid that TIME_WAIT prevents the server restarting.
  if (!sock::SetReuseAddr(acceptor_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEADDR");
    return false;
  }

  if (!sock::SetReuseAddr(acceptor_fd_)) {
    SPDLOG_ERROR("Failed to set SO_REUSEPORT");
    return false;
  }

  // Set the acceptor to be non-blocking to avoid calling accept() to block too much time.
  if (!sock::SetNonBlocking(acceptor_fd_)) {
    SPDLOG_ERROR("Faild to set socket to be non-blocking.");
    return false;
  }

  if (!sock::Bind(acceptor_fd_, CONFIG.port)) {
    SPDLOG_ERROR("Failed to bind port: {}.", CONFIG.port);
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
  HandlePendingTimers();

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

  Connection* new_conn = connection_pool_->Get();
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
    connection_pool_->Release(conn);
    return;
  }

  if (on_connected_) {
    on_connected_(new_conn);
  }
}

// In I/O thread.
void Server::HandleRead(Connection* conn) {
  MessagePtr request;
  if (!conn->HandleRead(&request)) {
    if (on_disconnected_) {
      on_disconnected_(conn);
    }

    connection_pool_->Release(conn);
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

// In I/O thread.
void Server::HandlePendingTimers() {
  std::vector<TimerPtr> timers;
  {
    std::lock_guard<std::mutex> lock(pending_timer_mutex_);
    timers.swap(pending_timers_);
  }

  for (TimerPtr timer : timers) {
    timer->Run();
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

void Server::HandleTimeWheelScheduler(TimerPtr timer) {
  {
    std::lock_guard<std::mutex> lock(pending_timer_mutex_);
    pending_timers_.push_back(timer);
  }

  // Wake up epoll_wait to handle pending timers.
  WakeUp();
}

void Server::WakeUp() {
  uint64_t one = 1;
  ssize_t n = write(wakener_fd_, &one, sizeof(one));
  if (n != sizeof(one)) {
    SPDLOG_ERROR("Failed to wakeup.");
  }
}

void Server::InitTimeWheelScheduler() {
  // Hour time wheel. 24 scales, 1 scale = 60 * 60 * 1000 ms.
  time_wheel_scheduler_.AppendTimeWheel(24, 60 * 60 * 1000, "HourTimeWheel");
  // Minute time wheel. 60 scales, 1 scale = 60 * 1000 ms.
  time_wheel_scheduler_.AppendTimeWheel(60, 60 * 1000, "MinuteTimeWheel");
  // Second time wheel. 60 scales, 1 scale = 1000 ms.
  time_wheel_scheduler_.AppendTimeWheel(60, 1000, "SecondTimeWheel");
  // Millisecond time wheel. 1000/timer_step_ms scales, 1 scale = timer_step ms.
  auto timer_step = time_wheel_scheduler_.timer_step_ms();
  time_wheel_scheduler_.AppendTimeWheel(1000 / timer_step, timer_step, "MillisecondTimeWheel");
}

}  // namespace epoll_server
