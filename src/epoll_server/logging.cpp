#include "epoll_server/logging.h"

#include <memory>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace epoll_server {

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
                 std::size_t rotate_size, std::size_t rotate_count,
                 const std::string& console_log_level) {
  using namespace spdlog::sinks;

  auto file_sink = std::make_shared<rotating_file_sink_mt>(file_name, rotate_size, rotate_count);
  file_sink->set_level(GetLogLevel(level));

  auto stderr_sink = std::make_shared<stderr_color_sink_mt>();
  stderr_sink->set_level(GetLogLevel(console_log_level));

  auto logger = std::make_shared<spdlog::logger>("logger", spdlog::sinks_init_list{file_sink, stderr_sink});
  logger->set_level(spdlog::level::trace);
  logger->flush_on(GetLogLevel(level));
  logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e %z] [T:%-5t] [P:%-5P] [%-5l] [%=30@] %v");

  spdlog::set_default_logger(logger);
}

}  // namespace epoll_server
