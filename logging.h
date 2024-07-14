#ifndef LOGGING_H
#define LOGGING_H
#include <exception>
#include <memory>
#include <sstream>

namespace runtime {

class InternalError : public std::exception {
  std::string what_;

public:
  InternalError(std::string &&msg) : what_(std::move(msg)) {}
  const char *what() const noexcept override { return what_.c_str(); }
};

namespace detail {
class LogFatal {
  std::ostringstream stream_;

public:
  LogFatal(const char *file, int line) {
    stream_ << file << ":" << line << ": ";
  }
  std::ostringstream &stream() { return stream_; }
  ~LogFatal() noexcept(false) { throw InternalError(stream_.str()); }
};

template <typename X, typename Y>
std::unique_ptr<std::string> LogCheckFormat(const X &x, const Y &y) {
  std::ostringstream os;
  // os << " (" << x << " vs. " << y << ") ";
  return std::make_unique<std::string>("");
}

#define TIGER_CHECK_FUNC(name, op)                                             \
  template <typename X, typename Y>                                            \
  std::unique_ptr<std::string> LogCheck##name(const X &x, const Y &y) {        \
    if (x op y)                                                                \
      return nullptr;                                                          \
    return LogCheckFormat(x, y);                                               \
  }

TIGER_CHECK_FUNC(_EQ, ==)
TIGER_CHECK_FUNC(_NE, !=)
} // namespace detail

#define CHECK_BINARY_OP(name, op, x, y)                                        \
  if (auto __tiger__log__err = ::runtime::detail::LogCheck##name(x, y))        \
  ::runtime::detail::LogFatal(__FILE__, __LINE__).stream()                     \
      << "Check failed: " << #x " " #op " " #y << *__tiger__log__err << ": "

#define CHECK(cond)                                                            \
  if (!(cond))                                                                 \
  ::runtime::detail::LogFatal(__FILE__, __LINE__).stream()                     \
      << "Check failed: (" << #cond << ") is false: "

#define CHECK_EQ(x, y) CHECK_BINARY_OP(_EQ, ==, x, y)
#define CHECK_NE(x, y) CHECK_BINARY_OP(_NE, !=, x, y)
} // namespace runtime
#endif
