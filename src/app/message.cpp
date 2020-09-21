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

  data_len = static_cast<uint16_t>(data.size());
  code = code_;
  crc32 = CalcCRC32(data_);
  data = std::move(data_);
  conn_ = conn;
  conn_timestamp_ = conn_->GetTimestamp();
}

bool Message::Valid() const {
  if (conn_ == nullptr) {
    return false;
  }

  if (conn_timestamp_ != conn_->GetTimestamp()) {
    SPDLOG_WARN("Expired message.");
    return false;
  }

  if (data_len != data.size()) {
    SPDLOG_DEBUG("Invaid Request");
    return false;
  }

  // 只有包头，CRC32值应为0。
  if (data_len == 0 && crc32 !=0) {
    SPDLOG_DEBUG("Invaid Request");
    return false;
  }

  if (data_len !=0) {
    // CRC32校验失败。
    if (crc32 != CalcCRC32(data)) {
      SPDLOG_DEBUG("Invaid Request");
      return false;
    }
  }

  return true;
}

void Message::Unpack(Connection* conn, const char header[8], std::string&& data_) {
  assert(conn != nullptr);

  conn_ = conn;
  conn_timestamp_ = conn->GetTimestamp();

  data_len = BytesToUint16(kLittleEndian, &header[0]);
  code = BytesToUint16(kLittleEndian, &header[2]);
  crc32 = BytesToUint32(kLittleEndian, &header[4]);
  data = std::move(data_);
}

}  // namespace app
