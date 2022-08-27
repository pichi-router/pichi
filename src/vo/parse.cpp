#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

template <> string parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto ret = string{v.GetString()};
  assertFalse(ret.empty(), PichiError::BAD_JSON, msg::STR_EMPTY);
  return ret;
}

template <> bool parse(json::Value const& v)
{
  assertTrue(v.IsBool(), PichiError::BAD_JSON, msg::BOOL_TYPE_ERROR);
  return v.GetBool();
}

template <> DelayMode parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == delay::RANDOM) return DelayMode::RANDOM;
  if (str == delay::FIXED) return DelayMode::FIXED;
  fail(PichiError::BAD_JSON, msg::DM_INVALID);
}

template <> AdapterType parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == type::DIRECT) return AdapterType::DIRECT;
  if (str == type::REJECT) return AdapterType::REJECT;
  if (str == type::SOCKS5) return AdapterType::SOCKS5;
  if (str == type::HTTP) return AdapterType::HTTP;
  if (str == type::SS) return AdapterType::SS;
  if (str == type::TUNNEL) return AdapterType::TUNNEL;
  if (str == type::TROJAN) return AdapterType::TROJAN;
  if (str == type::VMESS) return AdapterType::VMESS;
  if (str == type::TRANSPARENT) return AdapterType::TRANSPARENT;
  fail(PichiError::BAD_JSON, msg::AT_INVALID);
}

template <> CryptoMethod parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == method::RC4_MD5) return CryptoMethod::RC4_MD5;
  if (str == method::BF_CFB) return CryptoMethod::BF_CFB;
  if (str == method::AES_128_CTR) return CryptoMethod::AES_128_CTR;
  if (str == method::AES_192_CTR) return CryptoMethod::AES_192_CTR;
  if (str == method::AES_256_CTR) return CryptoMethod::AES_256_CTR;
  if (str == method::AES_128_CFB) return CryptoMethod::AES_128_CFB;
  if (str == method::AES_192_CFB) return CryptoMethod::AES_192_CFB;
  if (str == method::AES_256_CFB) return CryptoMethod::AES_256_CFB;
  if (str == method::CAMELLIA_128_CFB) return CryptoMethod::CAMELLIA_128_CFB;
  if (str == method::CAMELLIA_192_CFB) return CryptoMethod::CAMELLIA_192_CFB;
  if (str == method::CAMELLIA_256_CFB) return CryptoMethod::CAMELLIA_256_CFB;
  if (str == method::CHACHA20) return CryptoMethod::CHACHA20;
  if (str == method::SALSA20) return CryptoMethod::SALSA20;
  if (str == method::CHACHA20_IETF) return CryptoMethod::CHACHA20_IETF;
  if (str == method::AES_128_GCM) return CryptoMethod::AES_128_GCM;
  if (str == method::AES_192_GCM) return CryptoMethod::AES_192_GCM;
  if (str == method::AES_256_GCM) return CryptoMethod::AES_256_GCM;
  if (str == method::CHACHA20_IETF_POLY1305) return CryptoMethod::CHACHA20_IETF_POLY1305;
  if (str == method::XCHACHA20_IETF_POLY1305) return CryptoMethod::XCHACHA20_IETF_POLY1305;
  fail(PichiError::BAD_JSON, msg::CM_INVALID);
}

template <> BalanceType parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == balance::RANDOM) return BalanceType::RANDOM;
  if (str == balance::ROUND_ROBIN) return BalanceType::ROUND_ROBIN;
  if (str == balance::LEAST_CONN) return BalanceType::LEAST_CONN;
  fail(PichiError::BAD_JSON, msg::BA_INVALID);
}

template <> VMessSecurity parse(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == security::AUTO) return VMessSecurity::AUTO;
  if (str == security::NONE) return VMessSecurity::NONE;
  if (str == security::CHACHA20_IETF_POLY1305) return VMessSecurity::CHACHA20_IETF_POLY1305;
  if (str == security::AES_128_GCM) return VMessSecurity::AES_128_GCM;
  fail(PichiError::BAD_JSON, msg::SEC_INVALID);
}

template <> uint16_t parse(json::Value const& v)
{
  assertTrue(v.IsInt(), PichiError::BAD_JSON, msg::INT_TYPE_ERROR);
  auto uint16 = v.GetInt();
  assertTrue(uint16 >= numeric_limits<uint16_t>::min(), PichiError::BAD_JSON, msg::UINT16_INVALID);
  assertTrue(uint16 <= numeric_limits<uint16_t>::max(), PichiError::BAD_JSON, msg::UINT16_INVALID);
  return static_cast<uint16_t>(uint16);
}

template <> Endpoint parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(endpoint::HOST), PichiError::BAD_JSON, msg::MISSING_HOST_FIELD);
  assertTrue(v.HasMember(endpoint::PORT), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
  return makeEndpoint(parse<string>(v[endpoint::HOST]), parse<uint16_t>(v[endpoint::PORT]));
}

string parseNameOrPassword(json::Value const& v)
{
  auto ret = parse<string>(v);
  assertTrue(ret.size() < 256, PichiError::BAD_JSON, msg::TOO_LONG_NAME_PASSWORD);
  return ret;
}

vector<Endpoint> parseDestinantions(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertFalse(v.MemberCount() == 0, PichiError::BAD_JSON);

  auto ret = vector<Endpoint>{};
  transform(v.MemberBegin(), v.MemberEnd(), back_inserter(ret), [](auto&& item) {
    return makeEndpoint(parse<string>(item.name), parse<uint16_t>(item.value));
  });
  return ret;
}

}  // namespace pichi::vo
