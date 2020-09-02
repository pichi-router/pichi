#include <pichi/common/asserts.hpp>

using namespace std;

namespace pichi {

[[noreturn]] void fail(PichiError e, string_view msg) { throw Exception{e, msg}; }

[[noreturn]] void fail(string_view msg) { fail(PichiError::MISC, msg); }

void assertTrue(bool b, PichiError e, string_view msg)
{
  if (!b) fail(e, msg);
}

void assertTrue(bool b, string_view msg) { assertTrue(b, PichiError::MISC, msg); }

void assertFalse(bool b, PichiError e, string_view msg) { assertTrue(!b, e, msg); }

void assertFalse(bool b, string_view msg) { assertTrue(!b, PichiError::MISC, msg); }

} // namespace pichi
