#ifndef APP_CRC32_H_
#define APP_CRC32_H_

#include <string>

namespace app {

unsigned int CalcCRC32(const std::string& data_bytes);

}  // namespace app

#endif  // APP_CRC32_H_
