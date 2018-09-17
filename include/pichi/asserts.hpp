#ifndef PICHI_ASSERTS_HPP
#define PICHI_ASSERTS_HPP

#include <pichi/exception.hpp>
#include <string_view>

namespace pichi {

[[noreturn]] extern void fail(PichiError e, std::string_view msg = "");
extern void assertTrue(bool b, PichiError e, std::string_view msg = "");
extern void assertFalse(bool b, PichiError e, std::string_view msg = "");

} // namespace pichi

#endif
