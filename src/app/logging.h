#ifndef APP_LOGGING_H_
#define APP_LOGGING_H_

#include <string>

#include "spdlog/spdlog.h"

#define SPDLOG_TRACK_METHOD app::TrackMethodLog track_method_log(__FUNCTION__)

namespace app {

// Log level: "trace", "debug", "info", "warn", "error", "fatal".
// Rotate size: Byte.
void InitLogging(const std::string& file_name, const std::string& level,
                 std::size_t rotate_size, std::size_t rotate_count,
                 const std::string& console_log_level);

class TrackMethodLog {
public:
  explicit TrackMethodLog(const std::string& method) : method_(method) {
    SPDLOG_TRACE("<Enter> : {}()", method_);
  }

  ~TrackMethodLog() {
    SPDLOG_TRACE("<Leave> : {}()", method_);
  }

private:
  std::string method_;
};

}  // namespace app

#endif  // APP_LOGGING_H_
