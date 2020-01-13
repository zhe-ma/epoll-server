#include "app/logging.h"

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace app {

static spdlog::level::level_enum GetLogLevel(const std::string& level_name) {
  if (level_name == "trace") {
    return spdlog::level::trace;
  }

  if (level_name == "debug") {
    return spdlog::level::debug;
  }

  if (level_name == "info") {
    return spdlog::level::info;
  }

  if (level_name == "warn") {
    return spdlog::level::warn;
  }

  if (level_name == "error") {
    return spdlog::level::err;
  }

  if (level_name == "fatal") {
    return spdlog::level::critical;
  }

  return spdlog::level::debug;
}

void InitLogging(const std::string& file_name, const std::string& level,
                 std::size_t rotate_size, std::size_t rotate_count) {
  auto file_logger = spdlog::rotating_logger_mt("file_logger", file_name,
                                                rotate_size, rotate_count);
  file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e %z] [T:%-5t] [P:%-5P] "
                           "[%-5l] [%=30@] %v");

  auto log_level = GetLogLevel(level);
  file_logger->set_level(log_level);
  file_logger->flush_on(log_level);

  spdlog::set_default_logger(file_logger);
}

}  // namespace app
