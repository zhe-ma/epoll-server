#ifndef APP_UTILS_H_
#define APP_UTILS_H_

namespace app {

namespace sock {

bool SetNonBlocking(int fd);

bool SetClosexc(int fd);

bool Bind(int fd, unsigned short port);

bool SetReuseAddr(int fd);

}  // namespace sock

}  // namespace app

#endif  // APP_UTILS_H_
