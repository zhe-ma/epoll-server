#include "app/message.h"

#include <cassert>

#include "app/connection.h"
#include "app/crc32.h"
#include "app/utils.h"
#include "app/logging.h"

namespace app {

Message::Message()
    : conn_(nullptr)
    , conn_timestamp_(0)
    , data_len(0)
    , code(0)
    , crc32(0) {
}

Message::Message(Connection* conn, uint16_t code_, std::string&& data_) {\
  assert(conn != nullptr);

  data = std::move(data_);
  data_len = static_cast<uint16_t>(data.size());
  code = code_;
  crc32 = CalcCRC32(data);
  conn_ = conn;
  conn_timestamp_ = conn_->timestamp();
}

bool Message::Valid() const {
  if (conn_ == nullptr) {
    return false;
  }

  if (IsExpired()) {
    SPDLOG_WARN("Expired message.");
    return false;
  }

  if (data_len != data.size()) {
    SPDLOG_DEBUG("Invaid Request");
    return false;
  }

  // The CRC32 value should be 0 if no header.
  if (data_len == 0 && crc32 !=0) {
    SPDLOG_DEBUG("Invaid Request");
    return false;
  }

  if (data_len !=0  && crc32 != CalcCRC32(data)) {
    SPDLOG_DEBUG("Invaid Request");
    return false;
  }

  return true;
}

bool Message::IsExpired() const {
  if (conn_ == nullptr) {
    return true;
  }

  // The message is expired because the related conn_ is closed or reused by new connection.
  if (conn_timestamp_ != conn_->timestamp()) {
    return true;
  }

  return false;
}

void Message::Unpack(Connection* conn, const char header[8], std::string&& data_) {
  assert(conn != nullptr);

  conn_ = conn;
  conn_timestamp_ = conn->timestamp();

  data_len = BytesToUint16(kLittleEndian, &header[0]);
  code = BytesToUint16(kLittleEndian, &header[2]);
  crc32 = BytesToUint32(kLittleEndian, &header[4]);
  data = std::move(data_);
}

std::string Message::Pack() const {
  std::string buf;

  buf += Uint16ToBytes(kLittleEndian, data_len);
  buf += Uint16ToBytes(kLittleEndian, code);
  buf += Uint32ToBytes(kLittleEndian, crc32);
  buf += data;

  return buf;
}

}  // namespace app
