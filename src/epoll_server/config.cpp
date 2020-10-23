#include "epoll_server/config.h"

#include "epoll_server/json_helper.h"
#include "epoll_server/logging.h"

namespace epoll_server {

Config::Config()
    : log_file_level("debug")
    , log_console_level("fatal")
    , log_filename("log/Server.log")
    , log_rotate_size(5242880)
    , log_rotate_count(10)
    , deamon_mode(false)
    , master_worker_mode(false)
    , process_worker_count(2)
    , port(9527)
    , connection_pool_size(20000)
    , thread_pool_size(4)
    , max_data_length(3000)
    , master_title("ServerMaster")
    , worker_title("ServerWorker") {
}

void Config::Load(const std::string& file_path) {
  std::string error_msg;
  Json::Value config;
  if (!LoadJsonFromFile(file_path, &config, &error_msg)) {
    SPDLOG_ERROR("Failed to load config file:{}. Error:{}.", file_path, error_msg);
    return;
  }

  const Json::Value& log_config = config["log"];
  log_filename = log_config["filename"].asString();
  log_file_level = log_config["fileLevel"].asString();
  log_console_level = log_config["consoleLevel"].asString();
  log_rotate_size = log_config["rotateFileSize"].asUInt();
  log_rotate_count = log_config["rotateFileCount"].asUInt();

  const Json::Value& process_config = config["process"];
  master_worker_mode = process_config["masterWorkerMode"].asBool();
  process_worker_count = process_config["workerCount"].asUInt();
  deamon_mode = process_config["daemonMode"].asBool();
  master_title = process_config["masterTitle"].asString();
  worker_title = process_config["workerTitle"].asString();

  const Json::Value& socket_config = config["socket"];
  port = static_cast<std::uint16_t>(socket_config["port"].asUInt());
  connection_pool_size = socket_config["connectionPoolSize"].asUInt();
  thread_pool_size = socket_config["threadPoolSize"].asUInt();
  max_data_length = socket_config["maxDataLength"].asUInt();
}

}  // namespace epoll_server
