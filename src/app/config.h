#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include <string>

#include "app/singleton_base.h"

#define CONFIG (*app::Config::GetInstance())

namespace app {

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
};

}  // namespace app

#endif  // APP_CONFIG_H_
