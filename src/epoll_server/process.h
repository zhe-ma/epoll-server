#ifndef EPOLL_SERVER_PROCESS_H_
#define EPOLL_SERVER_PROCESS_H_

#include <string>

namespace epoll_server {

extern char** g_argv;
extern int g_argc;

void BackupEnviron();
bool SetProcessTitle(const std::string& title);

// Return value > 0  : parent process
// return value == 0 : child daemon process
// return value < 0  : fork error 
int CreateDaemonProcess();

}  // namespace epoll_server

#endif  // EPOLL_SERVER_PROCESS_H_
