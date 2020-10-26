#include "epoll_server/process.h"

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <memory>

#include "epoll_server/logging.h"

namespace epoll_server {

char** g_argv = nullptr;
int g_argc = 0;
bool g_reap = false;

static std::size_t s_environ_size = 0;
static std::size_t s_argv_size = 0;
static std::unique_ptr<char[]> s_backup_environ;

void BackupEnviron() {
  for (std::size_t i = 0; environ[i] != nullptr; ++i) {
    s_environ_size += strlen(environ[i]) + 1;
  }

  s_backup_environ = std::unique_ptr<char[]>(new char[s_environ_size]);
  char* backup_environ = s_backup_environ.get();
  
  memset(backup_environ, 0, s_environ_size);

  char* p = backup_environ;
  for (std::size_t i = 0; environ[i] != nullptr; ++i) {
    std::size_t len = strlen(environ[i]) + 1;
    strcpy(p, environ[i]);
    environ[i] = p;
    p += len;
  }

  for (std::size_t i = 0; g_argv[i] != nullptr ; i++) {
    s_argv_size += strlen(g_argv[i]) + 1;
  }
}

bool SetProcessTitle(const std::string& title) {
  if (title.empty()) {
    return true;
  }

  std::size_t max_name_size = s_environ_size + s_argv_size;
  if (title.size() >= max_name_size) {
    return false;
  }

  g_argv[1] = NULL;

  char* p = g_argv[0];
  strcpy(p, title.c_str());
  p += title.size();

  std::size_t left_size = max_name_size - title.size();
  memset(p, 0, left_size);

  return true;
}

int CreateDaemonProcess() {
  SPDLOG_TRACK_METHOD;

  pid_t pid = fork();

  // Fork error.
  if (pid < 0) {
    SPDLOG_ERROR("Failed to fork daemon process.");
    return pid;
  }

  // Parent process.
  if (pid > 0) {
    return pid;
  }

  // Child daemon process.

  if (setsid() == -1) {
    SPDLOG_ERROR("failed to setsid.");
    return -1;
  }

  umask(0);

  int fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    SPDLOG_ERROR("Failed to open /dev/null.");
    return -1;
  }

  if (dup2(fd, STDIN_FILENO) == -1) {
    SPDLOG_ERROR("Failed to dup2 STDIN_FILENO.");
    return -1;
  }

  if (dup2(fd, STDOUT_FILENO) == -1) {
    SPDLOG_ERROR("Failed to dup2 STDOUT_FILENO.");
    return -1;
  }

  // STDIN_FILENO 0 Standard input.
  // STDOUT_FILENO 1 Standard output.
  // STDERR_FILENO 2 Standard error output.
  // So fd must be greater than 2.
  if (fd > STDERR_FILENO) {
    if (close(fd) == -1) {
      SPDLOG_ERROR("Fialed to close fd.");
      return -1;
    }
  }

  return pid;
}

struct SignalAction {
  int signo;
  std::string signo_name;
  void (*handler)(int signo, siginfo_t* siginfo, void* ucontext);
};

void SignalHandler(int signo, siginfo_t* siginfo, void* ucontext);

const SignalAction kSignalActions[] = {
  { SIGHUP, "SIGHUP", SignalHandler },  // Terminal hangup.
  { SIGINT, "SIGINT", SignalHandler },  // Interactive attention signal. Ctrl+C.
  { SIGTERM, "SIGTERM", SignalHandler },  // Termination request.
  { SIGCHLD, "SIGCHLD", SignalHandler },  // Child terminated or stopped.
  { SIGQUIT, "SIGQUIT", SignalHandler },  // Quit. Generate core file. Ctrl+\.
  { SIGIO, "SIGIO", SignalHandler },  // I/O now possible.
  { SIGSYS, "SIGSYS", nullptr },  // Bad system call.
};

std::string GetSignalName(int signo) {
  for (const auto& sig_action : kSignalActions) {
    if (signo == sig_action.signo) {
      return sig_action.signo_name;
    }
  }

  return "";
}

void SignalHandler(int signo, siginfo_t* siginfo, void* ucontext) {
  SPDLOG_TRACK_METHOD;

  SPDLOG_DEBUG("Process[{}] receives signal[{}:{}].", getpid(), signo, GetSignalName(signo));

  // siginfo->si_pid: The process ID of the signal sender.
  if (siginfo != nullptr) {
    SPDLOG_DEBUG("Received signal [{}, {}] from PID [{}].", signo, GetSignalName(signo), siginfo->si_pid);
  }

  // Parent process should call wait or waitpid to avoid killed child process to be a defunct.
  if (signo != SIGCHLD) {
    return;
  }

  for (; ;) {
    bool once = false;
    int status = 0;
    pid_t pid = waitpid(WAIT_ANY, &status, WNOHANG);  // WNOHANG : Non-blocking.

    // The child hasn't ended.
    if (pid == 0) {
      SPDLOG_DEBUG("The child hasn't ended.");
      return;
    } else if (pid == -1) {
      int error = errno;
      SPDLOG_DEBUG("Failed to waitpid. Error: {}.", error);
      if (error == EINTR) {
        continue;
      }

      if (error == ECHILD && once) {
        g_reap = true;
      }

      return;
    }

    once = true;
    // Get the signal id that make child process end.
    if (WTERMSIG(status) > 0) {
      SPDLOG_DEBUG("Child process [PID : {}] exited on signal [{}].", pid, WTERMSIG(status));
    } else {
      SPDLOG_DEBUG("Child process [PID : {}] exited with error code [{}].", pid, WEXITSTATUS(status));
    }
  }
}

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

    SPDLOG_DEBUG("Sigaction signal {} successfully!", signal_action.signo_name);
  }

  return true;
}

void BlockMasterProcessSignals() {
  sigset_t set;
  sigemptyset(&set);

  sigaddset(&set, SIGCHLD);  // Child terminated or stopped.
  sigaddset(&set, SIGALRM);  // Alarm clock.
  sigaddset(&set, SIGIO);  // I/O now possible.
  sigaddset(&set, SIGINT);  // Interactive attention signal. Ctrl+C.
  sigaddset(&set, SIGHUP);  // Terminal hangup.
  sigaddset(&set, SIGUSR1);  // User-defined signal 1.
  sigaddset(&set, SIGUSR2);  // User-defined signal 2.
  sigaddset(&set, SIGWINCH); // Window size change.
  sigaddset(&set, SIGTERM);  // Termination request.
  sigaddset(&set, SIGQUIT);  // Quit. Generate core file. Ctrl+\.

  int ret = sigprocmask(SIG_BLOCK, &set, nullptr);
  SPDLOG_DEBUG("Master process blocks signals: {}.", ret);
}

}  // namespace epoll_server
