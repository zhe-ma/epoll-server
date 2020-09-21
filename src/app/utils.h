#ifndef APP_UTILS_H_
#define APP_UTILS_H_

#include <cstdint>

namespace app {

namespace sock {

bool SetNonBlocking(int fd);

bool SetClosexc(int fd);

bool Bind(int fd, unsigned short port);

bool SetReuseAddr(int fd);

}  // namespace sock

enum ByteOrder {
  kLittleEndian = 0,
  kBigEndian
};

uint32_t BytesToUint32(ByteOrder byte_order, const char bytes[4]);
uint16_t BytesToUint16(ByteOrder byte_order, const char bytes[2]);

// Return the milliseconds timestamp.
int64_t GetNowTimestamp();

}  // namespace app

#endif  // APP_UTILS_H_
