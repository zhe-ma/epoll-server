#include "app/message.h"

#include "app/utils.h"

namespace app {

Message::Message(char header[8], std::string&& data_)
    : data_len(BytesToUint16(kLittleEndian, &header[0]))
    , code(BytesToUint16(kLittleEndian, &header[2]))
    , crc32(BytesToUint32(kLittleEndian, &header[4]))
    , data(std::move(data_)) {
}

}  // namespace app
