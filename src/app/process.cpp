#include "app/process.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <memory>

#include "logging.h"

namespace app {

char** g_argv = nullptr;
int g_argc = 0;

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

  // setsid()调用成功后，返回新的会话的ID。
  // 调用setsid函数的进程成为新的会话的leader进程，并与其父进程的会话组和进程组脱离。
  // 由于会话对控制终端的独占性，进程同时与控制终端脱离。
  // 从而达到脱离终端，终端关闭，将跟此子进程无关的目的。
  if (setsid() == -1) {
    SPDLOG_ERROR("failed to setsid.");
    return -1;
  }

  // 权限掩码。0代表不掩码任何权限，设置允许当前进程创建文件或者目录最大可操作的权限。
  // 避免了创建目录或文件的权限不确定性。
  umask(0);

  // 以读写方式打开黑洞设备文件。
  int fd = open("/dev/null", O_RDWR);
  if (fd == -1) {
    SPDLOG_ERROR("Failed to open /dev/null.");
    return -1;
  }

  // dup2函数有2个参数fd1和fd2。如果文件描述符表的fd2条目是打开的，dup2将其关闭，并将fd1的指针拷贝到条目fd2中去。
  // dup2执行失败返回-1，执行成功返回被复制的文件描述符。
  // 使STDIN_FILENO重定向到/dev/null。
  if (dup2(fd, STDIN_FILENO) == -1) {
    SPDLOG_ERROR("Failed to dup2 STDIN_FILENO.");
    return -1;
  }

  // 使STDOUT_FILENO重定向到/dev/null。
  if (dup2(fd, STDOUT_FILENO) == -1) {
    SPDLOG_ERROR("Failed to dup2 STDOUT_FILENO.");
    return -1;
  }

  // STDIN_FILENO 0	Standard input.
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

}  // namespace app
