#include <iostream>

#include "app/config.h"
#include "app/logging.h"

using namespace app;

int main() {
  std::string error_msg;
  if (!CONFIG.Load("conf/config.json", &error_msg)) {
    SPDLOG_CRITICAL("Failed to load config file. Error: {}.", error_msg);
  }

  InitLogging(CONFIG.log_filename, CONFIG.log_level, CONFIG.log_rotate_size,
              CONFIG.log_rotate_count);

  return 0;
}