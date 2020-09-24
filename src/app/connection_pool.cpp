#include "app/connection_pool.h"

#include "app/connection.h"

namespace app {

ConnectionPool::ConnectionPool(size_t size) {
  for (std::size_t i = 0; i < size; ++i) {
    pool_.push_back(new Connection);
  }
}

ConnectionPool::~ConnectionPool() {
  for (Connection* conn : pool_) {
    delete conn;
  }
  pool_.clear();
}

Connection* ConnectionPool::Get() {
  if (Empty()) {
    return nullptr;
  }

  Connection* conn = pool_.front();
  if (conn == nullptr) {
    return nullptr;
  }

  conn->UpdateTimestamp();

  pool_.pop_front();
  return conn;
}

void ConnectionPool::Release(Connection* conn) {
  if (conn == nullptr) {
    return;
  }

  conn->Close();

  pool_.push_back(conn);
}

size_t ConnectionPool::Size() const {
  return pool_.size();
}

bool ConnectionPool::Empty() const {
  return pool_.empty();
}

}  // namespace app