#ifndef APP_UTILS_H_
#define APP_UTILS_H_

#include <cstdint>
#include <string>

namespace app {

namespace sock {

bool SetNonBlocking(int fd);

bool SetClosexc(int fd);

bool Bind(int fd, unsigned short port);

bool SetReuseAddr(int fd);

// Return > 0: Receiving data count.
// Return = 0: EAGAIN or EWOULDBLOCK or EINTR.
// Return = -1: Error.
int Recv(int fd, char* buf, size_t buf_len);

// Return > 0: Sended data count.
// Return = 0: EAGAIN or EWOULDBLOCK.
// Return = -1: Error.
int Send(int fd, const char* buf, size_t buf_len, size_t* sended_size);

}  // namespace sock

enum ByteOrder {
  kLittleEndian = 0,
  kBigEndian
};

uint32_t BytesToUint32(ByteOrder byte_order, const char bytes[4]);
uint16_t BytesToUint16(ByteOrder byte_order, const char bytes[2]);

std::string Uint32ToBytes(ByteOrder byte_order, uint32_t num);
std::string Uint16ToBytes(ByteOrder byte_order, uint16_t num);

// Return the milliseconds timestamp.
int64_t GetNowTimestamp();

}  // namespace app

#endif  // APP_UTILS_H_
