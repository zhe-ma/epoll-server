#include "app/connection_pool.h"

#include "app/connection.h"

namespace app {

ConnectionPool::ConnectionPool(size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    poll_.push_back(new Connection);
  }
}

ConnectionPool::~ConnectionPool() {
  for (Connection* conn : poll_) {
    delete conn;
  }
  poll_.clear();
}

Connection* ConnectionPool::Get() {
  if (Empty()) {
    return nullptr;
  }

  Connection* conn = poll_.front();
  poll_.pop_front();
  return conn;
}

void ConnectionPool::Release(Connection* conn) {
  if (conn == nullptr) {
    return;
  }

  conn->Close();

  poll_.push_back(conn);
}

size_t ConnectionPool::Size() const {
  return poll_.size();
}

bool ConnectionPool::Empty() const {
  return poll_.empty();
}

}  // namespace app