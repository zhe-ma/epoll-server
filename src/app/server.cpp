#include "app/server.h"

#include <unistd.h>

#include <iostream>

#include "app/logging.h"
#include "app/process.h"

namespace app {

namespace {

const std::string& kMasterProcessTitle = "master process";
const std::string& kWorkerProcessTitle = "worker process";

void StartWorkerProcesses(std::size_t worker_count) {
  SPDLOG_TRACK_METHOD;

  for (size_t i = 0; i < worker_count; ++i) {
    pid_t pid = fork();

    // Fork error.
    if (pid == -1) {
      SPDLOG_ERROR("Failed to fork worker process.");
      continue;
    };

    // "pid == 0": Child process will enter the branch.
    // Parent process will continue to fork the next child process.
    if (pid == 0) {
      pid_t child_pid = getpid();
      SPDLOG_DEBUG("Fork a new child process. PID: {}.", child_pid);

      SetProcessTitle(kWorkerProcessTitle);

      // The handling of child process.
      for (;;) {
        std::cout << "I'm child: " << child_pid << std::endl;
        sleep(3);
      }
    }
  }
}

} // namespace

void Server::Start(std::size_t worker_count) {
  SPDLOG_TRACK_METHOD;

  // Set title for master process.
  std::string title = kMasterProcessTitle;
  for (int i = 0; i < g_argc; ++i) {
    title += std::string(" ") + g_argv[i];
  }
  SetProcessTitle(title);
  SPDLOG_DEBUG("The title of master process: {}.", title);

  StartWorkerProcesses(worker_count);

  // The handling of master process.
  for (;;) {
    std::cout << "I'm parent: " << getpid() << std::endl;
    sleep(3);
  }
}

}  // namespace app
