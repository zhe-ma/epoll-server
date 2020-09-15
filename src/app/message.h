#ifndef APP_MESSAGE_H_
#define APP_MESSAGE_H_

#include <string>

namespace app {

// Message = Header + Body.
// Header = DataLength + MsgCode + Crc32.
// Message Bytes = DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.

struct Message {
  uint16_t data_len;  // 数据长度。
  uint16_t code;  // 区别不同的命令。
  int32_t crc32;  // 数据校验。

  std::string data;

  // HeaderLen = sizeof(data_len) + sizeof(code) + sizeof(crc32)
  const static uint16_t kHeaderLen = 8;

  Message(char header[8], std::string&& data_);
};

}  // namespace app

#endif  // APP_MESSAGE_H_
