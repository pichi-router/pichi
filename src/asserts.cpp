#include <pichi/asserts.hpp>

using namespace std;

namespace pichi {

[[noreturn]] void fail(PichiError e, string_view msg) {
  throw Exception{e, msg};
}

void assertTrue(bool b, PichiError e, string_view msg)
{
  if (!b) fail(e, msg);
}

void assertFalse(bool b, PichiError e, string_view msg) { assertTrue(!b, e, msg); }

} // namespace pichi
