#include "app/config.h"

#include "app/json_helper.h"

namespace app {

bool Config::Load(const std::string& file_path, std::string* error_msg) {
  Json::Value config;
  if (!LoadJsonFromFile(file_path, &config, error_msg)) {
    return false;
  }

  const Json::Value& log_config = config["log"];
  log_filename = log_config["filename"].asString();
  log_level = log_config["level"].asString();
  log_rotate_size = log_config["rotateFileSize"].asUInt();
  log_rotate_count = log_config["rotateFileCount"].asUInt();

  return true;
}

}  // namespace app
