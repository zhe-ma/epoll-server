#ifndef APP_MESSAGE_H_
#define APP_MESSAGE_H_

#include <string>
#include <memory>

namespace app {

class Connection;

// Message = Header + Body.
// Header = DataLength + MsgCode + Crc32.
// Message Bytes = DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.

class Message {
public:
  uint16_t data_len;  // 数据长度。
  uint16_t code;  // 区别不同的命令。
  uint32_t crc32;  // 数据校验。

  std::string data;

public:
  // HeaderLen = sizeof(data_len) + sizeof(code) + sizeof(crc32)
  const static uint16_t kHeaderLen = 8;

  Message();

  Message(Connection* conn, uint16_t code_, std::string&& data_);

  bool Valid() const;

  void Unpack(Connection* conn, const char header[8], std::string&& data_);

  Connection* conn() const {
    return conn_;
  }

  void set_conn(Connection* conn) {
    conn_ = conn;
  }

  void set_conn_timestamp(int64_t conn_timestamp) {
    conn_timestamp_ = conn_timestamp;
  }

private:
  Connection* conn_;

  // The connection may be expired, so use timestamp to idendify the original conection.
  int64_t conn_timestamp_;
};

using MessagePtr = std::shared_ptr<Message>;

}  // namespace app

#endif  // APP_MESSAGE_H_
