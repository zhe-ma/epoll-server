#include <iostream>

#include <unistd.h>

#include "jsoncpp/json/json.h"

#include "epoll_server/config.h"
#include "epoll_server/logging.h"
#include "epoll_server/process.h"
#include "epoll_server/server.h"
#include "epoll_server/router_base.h"

using namespace epoll_server;

class Router : public RouterBase {
  std::string HandleRequest(MessagePtr msg) override {
    SPDLOG_TRACE("Recv:{},{},{},{}", msg->data_len, msg->code, msg->crc32, msg->data);
    return "Ayou";
  }
};

// ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|epoll_server'
// netstat -anp | grep -E 'State|9000'

int main(int argc, char* const*argv) {
  // Load configuration.
  std::string error_msg;
  if (!CONFIG.Load("conf/config.json", &error_msg)) {
    SPDLOG_CRITICAL("Failed to load config file. Error: {}.", error_msg);
    return -1;
  }

  // TODO: console log configurable
  // Init logging.
  InitLogging(CONFIG.log_filename, CONFIG.log_level, CONFIG.log_rotate_size,
              CONFIG.log_rotate_count, "trace");
  SPDLOG_DEBUG("==========================================================");

  Server server(9005);
  server.AddRouter(2020, RouterPtr(new Router));
  server.Start();

  // // Backup environ and argv.
  // g_argv = (char**)argv;
  // g_argc = argc;
  // BackupEnviron();

  // if (CONFIG.deamon_mode) {
  //   // Daemon process.
  //   if (CreateDaemonProcess() == 0) {
  //     Server server;
  //     server.Start();
  //   }
  // } else {
  //   Server server;
  //   server.Start();
  // }

  return 0;
}

