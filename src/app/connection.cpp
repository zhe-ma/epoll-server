#include "app/connection.h"

#include "app/logging.h"

namespace app {

Connection::Connection(int socket_fd)
    : socket_fd_(socket_fd)
    , remote_port_(-1) {
}

void Connection::HandleRead() {
  read_handler_();  
}

void Connection::HandleWrite() {
  write_handler_();
}

}  // namespace app
