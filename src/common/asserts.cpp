#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/detail/throw_error.hpp>
#include <boost/system/system_error.hpp>
#include <errno.h>
#include <pichi/common/asserts.hpp>
#include <pichi/common/error.hpp>
#include <string>

using namespace std;
namespace sys = boost::system;
using ErrorCode = sys::error_code;
using SystemError = sys::system_error;

namespace pichi {

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

void assertTrue(bool b, ErrorCode const& ec)
{
  if (!b) boost::asio::detail::throw_error(ec);
}

[[noreturn]] void failWithErrno() { throw SystemError{ErrorCode{errno, sys::system_category()}}; }

void assertSuccess(int ret)
{
  if (ret == -1) failWithErrno();
}

}  // namespace pichi
