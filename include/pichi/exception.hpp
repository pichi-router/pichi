#ifndef PICHI_EXCEPTION_HPP
#define PICHI_EXCEPTION_HPP

#include <exception>
#include <string_view>
#include <string>

namespace pichi {

enum class PichiError { OK = 0, BAD_PROTO, CRYPTO_ERROR, BUFFER_OVERFLOW, MISC };

class Exception : public std::exception {
public:
  Exception(Exception const&);
  Exception(Exception&&);
  Exception& operator=(Exception const&) = delete;
  Exception& operator=(Exception&&) = delete;

public:
  explicit Exception(PichiError, std::string_view = "");
  ~Exception() override;

  char const* what() const noexcept override;

  PichiError error() const;

private:
  PichiError error_;
  std::string message_;
};

} // namespace pichi

#endif
