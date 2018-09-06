#ifndef PICHI_ASSERTS_HPP
#define PICHI_ASSERTS_HPP

#include <pichi/exception.hpp>
#include <string>

namespace pichi {

[[noreturn]] extern void fail(PichiError e, std::string const& msg = "");
extern void assertTrue(bool b, PichiError e, std::string const& msg = "");
extern void assertFalse(bool b, PichiError e, std::string const& msg = "");

} // namespace pichi

#endif
