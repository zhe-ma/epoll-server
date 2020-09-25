#ifndef EPOLL_SERVER_JSON_HELPER_H_
#define EPOLL_SERVER_JSON_HELPER_H_

#include <string>

#include "jsoncpp/json/json.h"

namespace epoll_server {

void JsonToString(const Json::Value& json, std::string* str);

bool StringToJson(const std::string& str, Json::Value* json,
                  std::string* error_msg = nullptr);

bool LoadJsonFromFile(const std::string& file_path, Json::Value* json,
                      std::string* error_msg = nullptr);

bool SaveJsonToFile(const std::string& file_path, const Json::Value& json);

}  // namespace epoll_server

#endif  // EPOLL_SERVER_JSON_HELPER_H_
