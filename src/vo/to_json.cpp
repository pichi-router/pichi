#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/to_json.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value portToJson(string const& port)
{
  auto p = std::stoi(port);
  assertTrue(p > 0 && p <= std::numeric_limits<uint16_t>::max());
  return json::Value{p};
}

json::Value toJson(string_view str, Allocator& alloc)
{
  auto ret = json::Value{};
  ret.SetString(str.data(), static_cast<json::SizeType>(str.size()), alloc);
  return ret;
}

json::Value toJson(DelayMode mode, Allocator& alloc)
{
  switch (mode) {
  case DelayMode::FIXED:
    return toJson(delay::FIXED, alloc);
  case DelayMode::RANDOM:
    return toJson(delay::RANDOM, alloc);
  default:
    fail();
  }
}

json::Value toJson(AdapterType type, Allocator& alloc)
{
  switch (type) {
  case AdapterType::DIRECT:
    return toJson(type::DIRECT, alloc);
  case AdapterType::REJECT:
    return toJson(type::REJECT, alloc);
  case AdapterType::SOCKS5:
    return toJson(type::SOCKS5, alloc);
  case AdapterType::HTTP:
    return toJson(type::HTTP, alloc);
  case AdapterType::SS:
    return toJson(type::SS, alloc);
  case AdapterType::TUNNEL:
    return toJson(type::TUNNEL, alloc);
  case AdapterType::TROJAN:
    return toJson(type::TROJAN, alloc);
  default:
    fail(PichiError::MISC);
  }
}

json::Value toJson(CryptoMethod method, Allocator& alloc)
{
  switch (method) {
  case CryptoMethod::RC4_MD5:
    return toJson(method::RC4_MD5, alloc);
  case CryptoMethod::BF_CFB:
    return toJson(method::BF_CFB, alloc);
  case CryptoMethod::AES_128_CTR:
    return toJson(method::AES_128_CTR, alloc);
  case CryptoMethod::AES_192_CTR:
    return toJson(method::AES_192_CTR, alloc);
  case CryptoMethod::AES_256_CTR:
    return toJson(method::AES_256_CTR, alloc);
  case CryptoMethod::AES_128_CFB:
    return toJson(method::AES_128_CFB, alloc);
  case CryptoMethod::AES_192_CFB:
    return toJson(method::AES_192_CFB, alloc);
  case CryptoMethod::AES_256_CFB:
    return toJson(method::AES_256_CFB, alloc);
  case CryptoMethod::CAMELLIA_128_CFB:
    return toJson(method::CAMELLIA_128_CFB, alloc);
  case CryptoMethod::CAMELLIA_192_CFB:
    return toJson(method::CAMELLIA_192_CFB, alloc);
  case CryptoMethod::CAMELLIA_256_CFB:
    return toJson(method::CAMELLIA_256_CFB, alloc);
  case CryptoMethod::CHACHA20:
    return toJson(method::CHACHA20, alloc);
  case CryptoMethod::SALSA20:
    return toJson(method::SALSA20, alloc);
  case CryptoMethod::CHACHA20_IETF:
    return toJson(method::CHACHA20_IETF, alloc);
  case CryptoMethod::AES_128_GCM:
    return toJson(method::AES_128_GCM, alloc);
  case CryptoMethod::AES_192_GCM:
    return toJson(method::AES_192_GCM, alloc);
  case CryptoMethod::AES_256_GCM:
    return toJson(method::AES_256_GCM, alloc);
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return toJson(method::CHACHA20_IETF_POLY1305, alloc);
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return toJson(method::XCHACHA20_IETF_POLY1305, alloc);
  default:
    fail(PichiError::MISC);
  }
}

json::Value toJson(BalanceType selector, Allocator& alloc)
{
  switch (selector) {
  case BalanceType::RANDOM:
    return toJson(balance::RANDOM, alloc);
  case BalanceType::ROUND_ROBIN:
    return toJson(balance::ROUND_ROBIN, alloc);
  case BalanceType::LEAST_CONN:
    return toJson(balance::LEAST_CONN, alloc);
  default:
    fail();
  }
}

json::Value toJson(VMessSecurity security, Allocator& alloc)
{
  switch (security) {
  case VMessSecurity::AUTO:
    return toJson(security::AUTO, alloc);
  case VMessSecurity::NONE:
    return toJson(security::NONE, alloc);
  case VMessSecurity::CHACHA20_IETF_POLY1305:
    return toJson(security::CHACHA20_IETF_POLY1305, alloc);
  case VMessSecurity::AES_128_GCM:
    return toJson(security::AES_128_GCM, alloc);
  default:
    fail();
  }
}

json::Value toJson(Endpoint const& endpoint, Allocator& alloc)
{
  auto ret = json::Value{json::kObjectType};
  ret.AddMember(endpoint::HOST, toJson(endpoint.host_, alloc), alloc);
  ret.AddMember(endpoint::PORT, endpoint.port_, alloc);
  return ret;
}

string toString(json::Value const& v)
{
  auto buf = json::StringBuffer{};
  auto writer = json::Writer<json::StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

}  // namespace pichi::vo
