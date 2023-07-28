#ifndef PICHI_COMMON_ASSERTS_HPP
#define PICHI_COMMON_ASSERTS_HPP

#include <boost/system/error_code.hpp>
#include <pichi/common/enumerations.hpp>
#include <string_view>

namespace pichi {

[[noreturn]] extern void fail(PichiError, std::string_view = "");

[[noreturn]] extern void fail(std::string_view = "");

extern void assertTrue(bool, PichiError e, std::string_view = "");

extern void assertTrue(bool, std::string_view = "");

extern void assertFalse(bool, PichiError e, std::string_view = "");

extern void assertFalse(bool, std::string_view = "");

extern void assertTrue(bool, boost::system::error_code const&);

// Assertions for system calls

[[noreturn]] extern void failWithErrno();

extern void assertSuccess(int);

template <typename T> void assertSuccess(T const* ptr)
{
  if (ptr == nullptr) failWithErrno();
}

}  // namespace pichi

#endif  // PICHI_COMMON_ASSERTS_HPP
