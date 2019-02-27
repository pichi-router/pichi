#include <pichi/exception.hpp>

using namespace std;

namespace pichi {

Exception::Exception(PichiError error, string_view message) : error_{error}, message_{message} {}

char const* Exception::what() const noexcept
{
  if (!message_.empty()) return message_.c_str();
  switch (error_) {
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
  case PichiError::MISC:
    return "Misc error";
  default:
    return "Unknown";
  }
}

PichiError Exception::error() const { return error_; }

} // namespace pichi
