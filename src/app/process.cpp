#include "app/process.h"

#include <unistd.h>
#include <string.h>

#include <memory>

namespace app {

char** g_argv = nullptr;

static size_t s_environ_size = 0;
static size_t s_argv_size = 0;
static std::unique_ptr<char[]> s_backup_environ;

bool SetProcessName(const std::string& name) {
  for (size_t i = 0; environ[i] != nullptr; ++i) {
    s_environ_size += strlen(environ[i]) + 1;
  }

  s_backup_environ = std::unique_ptr<char[]>(new char[s_environ_size]);
  char* backup_environ = s_backup_environ.get();
  
  memset(backup_environ, 0, s_environ_size);

  char* p = backup_environ;
  for (size_t i = 0; environ[i] != nullptr; ++i) {
    size_t len = strlen(environ[i]) + 1;
    strcpy(p, environ[i]);
    environ[i] = p;
    p += len;
  }

  size_t argv_len = 0;
  for (size_t i = 0; argv[i] != nullptr ; i++) {
    argv_len += strlen(argv[i]) + 1;
  }

  size_t max_name_len = argv_len + environ_len;
  if (name.size() >= max_name_len) {
    return false;
  }

    argv[1] = NULL;

    char *p = argv[0];
    strcpy(p,name.c_str());
    ptmp += ititlelen; //跳过标题

    //(5)把剩余的原argv以及environ所占的内存全部清0，否则会出现在ps的cmd列可能还会残余一些没有被覆盖的内容；
    size_t cha = esy - ititlelen;  //内存总和减去标题字符串长度(不含字符串末尾的\0)，剩余的大小，就是要memset的；
    memset(ptmp,0,cha);  
    return;
}


}  // namespace app
