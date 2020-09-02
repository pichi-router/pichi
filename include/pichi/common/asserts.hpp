#ifndef PICHI_COMMON_ASSERTS_HPP
#define PICHI_COMMON_ASSERTS_HPP

#include <pichi/common/exception.hpp>

namespace pichi {

[[noreturn]] extern void fail(PichiError e, std::string_view msg = "");
[[noreturn]] extern void fail(std::string_view msg = "");
extern void assertTrue(bool b, PichiError e, std::string_view msg = "");
extern void assertTrue(bool b, std::string_view msg = "");
extern void assertFalse(bool b, PichiError e, std::string_view msg = "");
extern void assertFalse(bool b, std::string_view msg = "");

} // namespace pichi

#endif // PICHI_COMMON_ASSERTS_HPP
