#include <iostream>

#include <unistd.h>
#include <thread>

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

// ps -eo pid,ppid,sid,tty,pgrp,comm,stat,cmd | grep -E 'bash|PID|Server'
// netstat -anp | grep -E 'State|9000'

int main(int argc, char* const*argv) {
  Server server;
  server.Init();

  server.set_on_connected([](Connection* conn) {
    SPDLOG_DEBUG("OnConnection: {} {}", conn->remote_ip(), conn->remote_port());
  });

  server.set_on_disconnected([](Connection* conn) {
    SPDLOG_DEBUG("OnDisconnection: {} {}", conn->remote_ip(), conn->remote_port());
  });

  server.AddRouter(2020, RouterPtr(new Router));

  std::thread t([&]{
    server.Start();
  });


  std::this_thread::sleep_for(std::chrono::seconds(1));
  auto timer_id = server.CreateTimerEvery(2000, []() {
    std::cout << "Timer 2s." << std::endl;
  });

  std::this_thread::sleep_for(std::chrono::seconds(6));
  server.CancelTimer(timer_id);

  std::this_thread::sleep_for(std::chrono::seconds(1000));


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

