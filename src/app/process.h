#ifndef APP_PROCESS_H_
#define APP_PROCESS_H_

#include <string>

namespace app {

extern char** g_argv;
extern int g_argc;

void BackupEnviron();
bool SetProcessTitle(const std::string& title);

// Return value > 0  : parent process
// return value == 0 : child daemon process
// return value < 0  : fork error 
int CreateDaemonProcess();

}  // namespace app

#endif  // APP_PROCESS_H_
