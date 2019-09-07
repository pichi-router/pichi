#ifndef PICHI_EXCEPTION_HPP
#define PICHI_EXCEPTION_HPP

#include <exception>
#include <string>
#include <string_view>

namespace pichi {

enum class PichiError {
  OK = 0,
  BAD_PROTO,
  CRYPTO_ERROR,
  BUFFER_OVERFLOW,
  BAD_JSON,
  SEMANTIC_ERROR,
  RES_IN_USE,
  RES_LOCKED,
  CONN_FAILURE,
  BAD_AUTH_METHOD,
  UNAUTHENTICATED,
  MISC
};

class Exception : public std::exception {
public:
  explicit Exception(PichiError, std::string_view = "");

  char const* what() const noexcept override;

  PichiError error() const;

private:
  PichiError error_;
  std::string message_;
};

} // namespace pichi

#endif
