#ifndef EPOLL_SERVER_MESSAGE_H_
#define EPOLL_SERVER_MESSAGE_H_

#include <string>
#include <memory>

namespace epoll_server {

class Connection;

// Message = Header + Body.
// Header = DataLength + MsgCode + Crc32.
// Message Bytes = DataLen(LittleEndian) + MsgCode(LittleEndian) + CRC32(LittleEndian) + Data.

class Message {
public:
  uint16_t data_len;  // Data length.
  uint16_t code;  // Distinguish commands.
  uint32_t crc32;  // Data check number.

  std::string data;

public:
  // HeaderLen = sizeof(data_len) + sizeof(code) + sizeof(crc32)
  const static uint16_t kHeaderLen = 8;

  Message();

  Message(Connection* conn, uint16_t code_, std::string&& data_);

  bool Valid() const;

  bool IsExpired() const;

  void Unpack(Connection* conn, const char header[8], std::string&& data_);

  std::string Pack() const;

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

}  // namespace epoll_server

#endif  // EPOLL_SERVER_MESSAGE_H_
