#include <boost/system/system_error.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/error.hpp>
#include <string>

using namespace std;
namespace sys = boost::system;
using ErrorCode = sys::error_code;
using SystemError = sys::system_error;

namespace pichi {

[[noreturn]] void fail(int e) { throw SystemError{ErrorCode{e, sys::system_category()}}; }

[[noreturn]] void fail(PichiError e, string_view msg)
{
  throw SystemError{makeErrorCode(e), string{cbegin(msg), cend(msg)}};
}

[[noreturn]] void fail(string_view msg) { fail(PichiError::MISC, msg); }

void assertTrue(bool b, PichiError e, string_view msg)
{
  if (!b) fail(e, msg);
}

void assertTrue(bool b, string_view msg) { assertTrue(b, PichiError::MISC, msg); }

void assertFalse(bool b, PichiError e, string_view msg) { assertTrue(!b, e, msg); }

void assertFalse(bool b, string_view msg) { assertTrue(!b, PichiError::MISC, msg); }

}  // namespace pichi
