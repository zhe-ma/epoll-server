#ifndef EPOLL_SERVER_CRC32_H_
#define EPOLL_SERVER_CRC32_H_

#include <string>

namespace epoll_server {

unsigned int CalcCRC32(const std::string& data_bytes);

}  // namespace epoll_server

#endif  // EPOLL_SERVER_CRC32_H_
