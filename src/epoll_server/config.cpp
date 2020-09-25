#include "epoll_server/config.h"

#include "epoll_server/json_helper.h"

namespace epoll_server {

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

  const Json::Value& process_config = config["process"];
  process_worker_count = process_config["workerCount"].asUInt();
  deamon_mode = process_config["daemonMode"].asBool();

  const Json::Value& socket_config = config["socket"];
  port = static_cast<std::uint16_t>(socket_config["port"].asUInt());

  return true;
}

}  // namespace epoll_server
