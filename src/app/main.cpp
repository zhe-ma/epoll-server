#include <iostream>

#include "app/config.h"
#include "app/logging.h"
#include "app/process.h"
#include "app/server.h"

using namespace app;

// ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|epoll_server'

int main(int argc, char* const*argv) {
  // Load configuration.
  std::string error_msg;
  if (!CONFIG.Load("conf/config.json", &error_msg)) {
    SPDLOG_CRITICAL("Failed to load config file. Error: {}.", error_msg);
    return -1;
  }

  g_argv = (char**)argv;
  BackupEnviron();

  // Init logging.
  InitLogging(CONFIG.log_filename, CONFIG.log_level, CONFIG.log_rotate_size,
              CONFIG.log_rotate_count);

  SetProcessTitle("App Server");

  StartServer();

  return 0;
}

