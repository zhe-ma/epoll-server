#ifndef APP_PROCESS_H_
#define APP_PROCESS_H_

#include <string>

namespace app {

extern char** g_argv;

bool SetProcessName(const std::string& name);

}  // namespace app

#endif  // APP_PROCESS_H_
