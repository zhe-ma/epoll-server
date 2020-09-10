#include "app/connection.h"

#include "app/logging.h"

namespace app {

Connection::Connection(int socket_fd)
    : socket_fd_(socket_fd) {
}

void Connection::HandleRead() {
  SPDLOG_DEBUG("Handle read");
}

void Connection::HandleWrite() {
  SPDLOG_DEBUG("Handle write");
}

}  // namespace app
