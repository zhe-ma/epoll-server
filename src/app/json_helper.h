#ifndef APP_JSON_HELPER_H_
#define APP_JSON_HELPER_H_

#include <string>

#include "jsoncpp/json/json.h"

namespace app {

void JsonToString(const Json::Value& json, std::string* str);

bool StringToJson(const std::string& str, Json::Value* json,
                  std::string* error_msg = nullptr);

bool LoadJsonFromFile(const std::string& file_path, Json::Value* json,
                      std::string* error_msg = nullptr);

bool SaveJsonToFile(const std::string& file_path, const Json::Value& json);

}  // namespace app

#endif  // APP_JSON_HELPER_H_
