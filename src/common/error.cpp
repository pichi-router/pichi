#include <boost/system/error_category.hpp>
#include <boost/system/error_condition.hpp>
#include <pichi/common/error.hpp>
#include <string>

using namespace std;
namespace sys = boost::system;

namespace pichi {

using ErrorCode = sys::error_code;
using ErrorCondition = sys::error_condition;

struct ErrorCategory : public sys::error_category {
  char const* name() const noexcept override { return "pichi"; }

  string message(int ev) const override
  {
    switch (static_cast<PichiError>(ev)) {
    case PichiError::OK:
      return "OK";
    case PichiError::BAD_PROTO:
      return "Bad protocol";
    case PichiError::CRYPTO_ERROR:
      return "Crypto error";
    case PichiError::BUFFER_OVERFLOW:
      return "Buffer maximum exceeded";
    case PichiError::BAD_JSON:
      return "Invalid JSON";
    case PichiError::SEMANTIC_ERROR:
      return "JSON semantic error";
    case PichiError::RES_IN_USE:
      return "Resource in use";
    case PichiError::RES_LOCKED:
      return "Resource locked";
    case PichiError::CONN_FAILURE:
      return "Connection failure";
    case PichiError::BAD_AUTH_METHOD:
      return "Bad authentication method";
    case PichiError::UNAUTHENTICATED:
      return "Authentication failure";
    case PichiError::MISC:
      return "Misc error";
    default:
      return "Unknown";
    }
  }

  ErrorCondition default_error_condition(int ev) const noexcept override { return {ev, *this}; }

  bool equivalent(int ev, ErrorCondition const& condition) const noexcept override
  {
    return condition.value() == ev && addressof(condition.category()) == this;
  }

  bool equivalent(ErrorCode const& ec, int ev) const noexcept override
  {
    return ec.value() == ev && addressof(ec.category()) == this;
  }
};

static auto const GLOBAL_CATEGORY = ErrorCategory{};

sys::error_category const& PICHI_CATEGORY = GLOBAL_CATEGORY;

ErrorCode make_error_code(PichiError e) { return makeErrorCode(e); }

ErrorCode makeErrorCode(PichiError e)
{
  return ErrorCode{static_cast<underlying_type_t<PichiError>>(e), GLOBAL_CATEGORY};
}

}  // namespace pichi
