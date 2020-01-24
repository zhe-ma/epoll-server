#include "app/server.h"

#include <unistd.h>
#include <signal.h>

#include <iostream>

#include "app/logging.h"
#include "app/process.h"

namespace app {

namespace {

const std::string& kMasterProcessTitle = "master process";
const std::string& kWorkerProcessTitle = "worker process";

void StartWorkerProcesses(std::size_t worker_count) {
  SPDLOG_TRACK_METHOD;

  for (size_t i = 0; i < worker_count; ++i) {
    pid_t pid = fork();

    // Fork error.
    if (pid == -1) {
      SPDLOG_ERROR("Failed to fork worker process.");
      continue;
    };

    // "pid == 0": Child process will enter the branch.
    // Parent process will continue to fork the next child process.
    if (pid == 0) {
      pid_t child_pid = getpid();
      SPDLOG_DEBUG("Fork a new child process. PID: {}.", child_pid);

      sigset_t set;
      sigemptyset(&set);
      // Unmask all signals masked by master process.
      int ret = sigprocmask(SIG_SETMASK, &set, nullptr);
      SPDLOG_DEBUG("Unmask all signals masked by master process: {}.", ret);

      SetProcessTitle(kWorkerProcessTitle);

      // The handling of child process.
      for (;;) {
        std::cout << "I'm child: " << child_pid << std::endl;
        sleep(3);
      }
    }
  }
}

void SetMasterProcessTitle() {
  std::string title = kMasterProcessTitle;
  for (int i = 0; i < g_argc; ++i) {
    title += std::string(" ") + g_argv[i];
  }
  SetProcessTitle(title);

  SPDLOG_DEBUG("The title of master process: {}.", title);
}

// -----------------------------------------------------------------------------

struct SignalAction {
  int signo;
  std::string signo_name;
  void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
};

void SignalHandler(int signo, siginfo_t* siginfo, void* ucontext) {
  std::cout << "Signal: " << signo << std::endl;
}

const SignalAction kSignalActions[] = {
  { SIGHUP, "SIGHUP", SignalHandler },  // 终止进程，终端线路挂断。
  { SIGINT, "SIGINT", SignalHandler },  // 终止进程，中断进程，Ctrl+C。
  { SIGTERM, "SIGTERM", SignalHandler },  // 终止进程，软件终止信号。
  { SIGCHLD, "SIGCHLD", SignalHandler },  // 当子进程停止或退出时通知父进程。
  { SIGQUIT, "SIGQUIT", SignalHandler },  // 建立CORE文件终止进程，并且生成core文件，Ctrl+\。
  { SIGIO, "SIGIO", SignalHandler },  // 通用异步I/O信号。
  { SIGSYS, "SIGSYS", nullptr },  // 非法的系统调用。忽略该信号，防止进程被操作系统杀掉。
};

bool InitSignals() {
  SPDLOG_TRACK_METHOD;

  for (const auto& signal_action : kSignalActions) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));

    if (signal_action.handler == nullptr) {
      // Ignore signal.
      sa.sa_handler = SIG_IGN;
    } else {
      // Invoke signal-catching function with three arguments instead of one.
      sa.sa_flags = SA_SIGINFO;
      sa.sa_sigaction = signal_action.handler;
    }

    // Empty the mask not to block any signal.
    sigemptyset(&sa.sa_mask);   

    if (sigaction(signal_action.signo, &sa, nullptr) == -1) {
      SPDLOG_CRITICAL("Failed to sigaction: {}.", signal_action.signo_name);
      return false;
    }
  }

  return true;
}

void BlockMasterProcessSignals() {
  sigset_t set;
  sigemptyset(&set);

  sigaddset(&set, SIGCHLD);  // 当子进程停止或退出时通知父进程。
  sigaddset(&set, SIGALRM);  // 定时器超时。
  sigaddset(&set, SIGIO);  // 异步I/O信号。
  sigaddset(&set, SIGINT);  // 终止进程，中断进程，Ctrl+C。
  sigaddset(&set, SIGHUP);  // 终止进程，终端线路挂断。
  sigaddset(&set, SIGUSR1);  // 用户定义信号。
  sigaddset(&set, SIGUSR2);  // 用户定义信号。
  sigaddset(&set, SIGWINCH); // 终端窗口大小改变。
  sigaddset(&set, SIGTERM);  // 终止进程，软件终止信号。
  sigaddset(&set, SIGQUIT);  // 建立CORE文件终止进程，并且生成core文件，Ctrl+\。

  int ret = sigprocmask(SIG_BLOCK, &set, nullptr);
  SPDLOG_DEBUG("Master process blocks signals: {}.", ret);
}

} // namespace

bool Server::Start(std::size_t worker_count) {
  SPDLOG_TRACK_METHOD;

  SetMasterProcessTitle();

  if (!InitSignals()) {
    return false;
  }

  SPDLOG_DEBUG("Init signals sucessfully.");

  // Block signals before call fork().
  BlockMasterProcessSignals();

  StartWorkerProcesses(worker_count);

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
    std::cout << "I'm parent: " << getpid() << std::endl;
  }

  return true;
}

}  // namespace app
