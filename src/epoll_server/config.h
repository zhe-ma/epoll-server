#ifndef EPOLL_SERVER_CONFIG_H_
#define EPOLL_SERVER_CONFIG_H_

#include <string>

#include "epoll_server/singleton_base.h"

#define CONFIG (*epoll_server::Config::GetInstance())

namespace epoll_server {

class Config : public SingletonBase<Config>{
public:
  bool Load(const std::string& file_path, std::string* error_msg = nullptr);

private:
  Config() = default;
  friend class SingletonBase<Config>;

public:
  // Log config.
  std::string log_filename;
  std::string log_level;
  std::size_t log_rotate_count;
  std::size_t log_rotate_size;

  // Process config.
  std::size_t process_worker_count;
  bool deamon_mode;

  // Socket config.
  std::uint16_t port;
};

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CONFIG_H_
