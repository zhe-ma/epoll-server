#include "app/message.h"

#include "app/utils.h"

namespace app {

Message::Message()
    : data_len(0)
    , code(0)
    , crc32(0)
    , valid_(false) {
}

void Message::Set(const char header[8], std::string&& data_) {
  data_len = BytesToUint16(kLittleEndian, &header[0]);
  code = BytesToUint16(kLittleEndian, &header[2]);
  crc32 = static_cast<int32_t>(BytesToUint32(kLittleEndian, &header[4]));
  data = std::move(data_);

  valid_ = true;
}

}  // namespace app
