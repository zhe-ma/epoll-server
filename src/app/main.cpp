#include <iostream>

#include "app/config.h"
#include "app/logging.h"
#include "app/process.h"
#include "app/server.h"

using namespace app;

// ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|epoll_server'
// netstat -anp | grep -E 'State|9000'

int main(int argc, char* const*argv) {
  // Load configuration.
  std::string error_msg;
  if (!CONFIG.Load("conf/config.json", &error_msg)) {
    SPDLOG_CRITICAL("Failed to load config file. Error: {}.", error_msg);
    return -1;
  }

  // Backup environ and argv.
  g_argv = (char**)argv;
  g_argc = argc;
  BackupEnviron();

  // Init logging.
  InitLogging(CONFIG.log_filename, CONFIG.log_level, CONFIG.log_rotate_size,
              CONFIG.log_rotate_count);
  SPDLOG_DEBUG("==========================================================");

  if (CONFIG.deamon_mode) {
    // Daemon process.
    if (CreateDaemonProcess() == 0) {
      Server server;
      server.Start();
    }
  } else {
    Server server;
    server.Start();
  }

  return 0;
}

