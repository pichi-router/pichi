#include <pichi/common/config.hpp>
// Include config.hpp first
#include <errno.h>
#include <pichi/common/asserts.hpp>
#include <stdlib.h>
#include <string.h>

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

void assertSyscallSuccess(int r, PichiError e)
{
  if (r != -1) return;
  auto msg = string{};
  msg.resize(1024);
#if defined(HAS_STRERROR_S)
  strerror_s(msg.data(), msg.size(), errno);
#elif defined(HAS_STRERROR_R)
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif  // __GNUC__
  strerror_r(errno, msg.data(), msg.size());
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__
#else
  msg = string{strerror(errno)};
#endif
  fail(e, msg);
}

}  // namespace pichi
