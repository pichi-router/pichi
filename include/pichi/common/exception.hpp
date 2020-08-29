#ifndef PICHI_COMMON_EXCEPTION_HPP
#define PICHI_COMMON_EXCEPTION_HPP

#include <exception>
#include <pichi/common/enums.hpp>
#include <string>
#include <string_view>

namespace pichi {

class Exception : public std::exception {
public:
  explicit Exception(PichiError, std::string_view = "");

  char const* what() const noexcept override;

  PichiError error() const;

private:
  PichiError error_;
  std::string message_;
};

}  // namespace pichi

#endif  // PICHI_COMMON_EXCEPTION_HPP
