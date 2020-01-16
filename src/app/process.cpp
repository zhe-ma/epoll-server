#include "app/process.h"

#include <unistd.h>
#include <string.h>

#include <memory>

namespace app {

char** g_argv = nullptr;

static size_t s_environ_size = 0;
static size_t s_argv_size = 0;
static std::unique_ptr<char[]> s_backup_environ;

void BackupEnviron() {
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

  for (size_t i = 0; g_argv[i] != nullptr ; i++) {
    s_argv_size += strlen(g_argv[i]) + 1;
  }
}

bool SetProcessTitle(const std::string& title) {
  size_t max_name_size = s_environ_size + s_argv_size;
  if (title.size() >= max_name_size) {
    return false;
  }

  g_argv[1] = NULL;

  char* p = g_argv[0];
  strcpy(p, title.c_str());
  p += title.size();

  size_t left_size = max_name_size - title.size();
  memset(p, 0, left_size);

  return true;
}

}  // namespace app
