#include "app/message.h"

#include "app/utils.h"

namespace app {

Message::Message()
    : conn_(nullptr)
    , data_len(0)
    , code(0)
    , crc32(0) {
}

bool Message::Valid() const {
  return conn_ != nullptr;
}

void Message::Set(Connection* conn, const char header[8], std::string&& data_) {
  conn_ = conn;
  data_len = BytesToUint16(kLittleEndian, &header[0]);
  code = BytesToUint16(kLittleEndian, &header[2]);
  crc32 = static_cast<int32_t>(BytesToUint32(kLittleEndian, &header[4]));
  data = std::move(data_);
}

}  // namespace app
