#ifndef EPOLL_SERVER_CONFIG_H_
#define EPOLL_SERVER_CONFIG_H_

#include <string>

#include "epoll_server/singleton_base.h"

#define CONFIG (*epoll_server::Config::GetInstance())

namespace epoll_server {

class Config : public SingletonBase<Config>{
public:
  void Load(const std::string& file_path);

private:
  Config();
  friend class SingletonBase<Config>;

public:
  // Log config.
  std::string log_filename;
  std::string log_file_level;
  std::string log_console_level;
  std::size_t log_rotate_count;
  std::size_t log_rotate_size;

  // Process config.
  bool master_worker_mode;
  size_t process_worker_count;
  bool deamon_mode;
  std::string master_title;
  std::string worker_title;

  // Socket config.
  uint16_t port;
  uint32_t connection_pool_size;
  uint32_t thread_pool_size;
  uint32_t max_data_length;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CONFIG_H_
