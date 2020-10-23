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

int main(int argc, char* const* argv) {
  Server server;
  server.Init(argc, (char**)argv);

  server.set_on_connected([](Connection* conn) {
    SPDLOG_DEBUG("OnConnection: {} {}", conn->remote_ip(), conn->remote_port());
  });

  server.set_on_disconnected([](Connection* conn) {
    SPDLOG_DEBUG("OnDisconnection: {} {}", conn->remote_ip(), conn->remote_port());
  });

  server.AddRouter(2020, RouterPtr(new Router));

  std::thread t([&](){
    server.Start();
  });

  server.CreateTimerEvery(10000, []() {
    std::cout << getpid() << ": Timer 10s." << std::endl;
  });

  sleep(100);

  return 0;
}

