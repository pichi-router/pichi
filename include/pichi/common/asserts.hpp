#ifndef PICHI_COMMON_ASSERTS_HPP
#define PICHI_COMMON_ASSERTS_HPP

#include <pichi/common/enumerations.hpp>
#include <string_view>

namespace pichi {

[[noreturn]] extern void fail(PichiError, std::string_view = "");

[[noreturn]] extern void fail(std::string_view = "");

extern void assertTrue(bool b, PichiError e, std::string_view = "");

extern void assertTrue(bool b, std::string_view = "");

extern void assertFalse(bool b, PichiError e, std::string_view = "");

extern void assertFalse(bool b, std::string_view = "");

// Assertions for system calls

[[noreturn]] extern void failWithErrno();

extern void assertSuccess(int);

template <typename T> void assertSuccess(T const* ptr)
{
  if (ptr == nullptr) failWithErrno();
}

}  // namespace pichi

#endif  // PICHI_COMMON_ASSERTS_HPP
