#ifndef APP_NONCOPYABLE_H_
#define APP_NONCOPYABLE_H_

namespace app {

class Noncopyable {
public:
  Noncopyable(const Noncopyable&) = delete;
  Noncopyable& operator=(const Noncopyable&) = delete;

protected:
  Noncopyable() = default;
  ~Noncopyable() = default;
};

}  // namespace app

#endif  // APP_CONNECTION_POLL_H_
