#include <limits>
#include <pichi/api/rest.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi {
namespace api {

static decltype(auto) DIRECT_TYPE = "direct";
static decltype(auto) REJECT_TYPE = "reject";
static decltype(auto) SOCKS5_TYPE = "socks5";
static decltype(auto) HTTP_TYPE = "http";
static decltype(auto) SS_TYPE = "ss";

static decltype(auto) RC4_MD5_METHOD = "rc4-md5";
static decltype(auto) BF_CFB_METHOD = "bf-cfb";
static decltype(auto) AES_128_CTR_METHOD = "aes-128-ctr";
static decltype(auto) AES_192_CTR_METHOD = "aes-192-ctr";
static decltype(auto) AES_256_CTR_METHOD = "aes-256-ctr";
static decltype(auto) AES_128_CFB_METHOD = "aes-128-cfb";
static decltype(auto) AES_192_CFB_METHOD = "aes-192-cfb";
static decltype(auto) AES_256_CFB_METHOD = "aes-256-cfb";
static decltype(auto) CAMELLIA_128_CFB_METHOD = "camellia-128-cfb";
static decltype(auto) CAMELLIA_192_CFB_METHOD = "camellia-192-cfb";
static decltype(auto) CAMELLIA_256_CFB_METHOD = "camellia-256-cfb";
static decltype(auto) CHACHA20_METHOD = "chacha20";
static decltype(auto) SALSA20_METHOD = "salsa20";
static decltype(auto) CHACHA20_IETF_METHOD = "chacha20-ietf";
static decltype(auto) AES_128_GCM_METHOD = "aes-128-gcm";
static decltype(auto) AES_192_GCM_METHOD = "aes-192-gcm";
static decltype(auto) AES_256_GCM_METHOD = "aes-256-gcm";
static decltype(auto) CHACHA20_IETF_POLY1305_METHOD = "chacha20-ietf-poly1305";
static decltype(auto) XCHACHA20_IETF_POLY1305_METHOD = "xchacha20-ietf-poly1305";

namespace IngressVOKey {

static decltype(auto) type_ = "type";
static decltype(auto) bind_ = "bind";
static decltype(auto) port_ = "port";
static decltype(auto) method_ = "method";
static decltype(auto) password_ = "password";

} // namespace IngressVOKey

namespace OutboundVOKey {

static decltype(auto) type_ = "type";
static decltype(auto) host_ = "host";
static decltype(auto) port_ = "port";
static decltype(auto) method_ = "method";
static decltype(auto) password_ = "password";

} // namespace OutboundVOKey

namespace RuleVOKey {

static decltype(auto) outbound_ = "outbound";
static decltype(auto) range_ = "range";
static decltype(auto) ingress_ = "ingress_name";
static decltype(auto) type_ = "ingress_type";
static decltype(auto) pattern_ = "pattern";
static decltype(auto) domain_ = "domain";
static decltype(auto) country_ = "country";

} // namespace RuleVOKey

namespace RouteVOKey {

static decltype(auto) default_ = "default";
static decltype(auto) rules_ = "rules";

} // namespace RouteVOKey

static AdapterType parseAdapterType(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::MISC);
  auto str = string_view{v.GetString()};
  if (str == DIRECT_TYPE) return AdapterType::DIRECT;
  if (str == REJECT_TYPE) return AdapterType::REJECT;
  if (str == SOCKS5_TYPE) return AdapterType::SOCKS5;
  if (str == HTTP_TYPE) return AdapterType::HTTP;
  if (str == SS_TYPE) return AdapterType::SS;
  fail(PichiError::MISC);
}

static CryptoMethod parseCryptoMethod(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::MISC);
  auto str = string_view{v.GetString()};
  if (str == RC4_MD5_METHOD) return CryptoMethod::RC4_MD5;
  if (str == BF_CFB_METHOD) return CryptoMethod::BF_CFB;
  if (str == AES_128_CTR_METHOD) return CryptoMethod::AES_128_CTR;
  if (str == AES_192_CTR_METHOD) return CryptoMethod::AES_192_CTR;
  if (str == AES_256_CTR_METHOD) return CryptoMethod::AES_256_CTR;
  if (str == AES_128_CFB_METHOD) return CryptoMethod::AES_128_CFB;
  if (str == AES_192_CFB_METHOD) return CryptoMethod::AES_192_CFB;
  if (str == AES_256_CFB_METHOD) return CryptoMethod::AES_256_CFB;
  if (str == CAMELLIA_128_CFB_METHOD) return CryptoMethod::CAMELLIA_128_CFB;
  if (str == CAMELLIA_192_CFB_METHOD) return CryptoMethod::CAMELLIA_192_CFB;
  if (str == CAMELLIA_256_CFB_METHOD) return CryptoMethod::CAMELLIA_256_CFB;
  if (str == CHACHA20_METHOD) return CryptoMethod::CHACHA20;
  if (str == SALSA20_METHOD) return CryptoMethod::SALSA20;
  if (str == CHACHA20_IETF_METHOD) return CryptoMethod::CHACHA20_IETF;
  if (str == AES_128_GCM_METHOD) return CryptoMethod::AES_128_GCM;
  if (str == AES_192_GCM_METHOD) return CryptoMethod::AES_192_GCM;
  if (str == AES_256_GCM_METHOD) return CryptoMethod::AES_256_GCM;
  if (str == CHACHA20_IETF_POLY1305_METHOD) return CryptoMethod::CHACHA20_IETF_POLY1305;
  if (str == XCHACHA20_IETF_POLY1305_METHOD) return CryptoMethod::XCHACHA20_IETF_POLY1305;
  fail(PichiError::MISC);
}

static uint16_t parsePort(json::Value const& v)
{
  assertTrue(v.IsInt(), PichiError::MISC);
  auto port = v.GetInt();
  assertTrue(port > 0, PichiError::MISC);
  assertTrue(port <= numeric_limits<uint16_t>::max(), PichiError::MISC);
  return static_cast<uint16_t>(port);
}

static string parseString(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::MISC);
  auto ret = string{v.GetString()};
  assertFalse(ret.empty(), PichiError::MISC);
  return ret;
}

template <typename OutputIt, typename T, typename Convert>
void parseArray(json::Value const& root, T const& key, OutputIt out, Convert&& convert)
{
  if (!root.HasMember(key)) return;
  assertTrue(root[key].IsArray(), PichiError::MISC);
  auto array = root[key].GetArray();
  transform(begin(array), end(array), out, forward<Convert>(convert));
}

json::Value toJson(string_view str, Allocator& alloc)
{
  auto ret = json::Value{};
  ret.SetString(str.data(), str.size(), alloc);
  return ret;
}

json::Value toJson(AdapterType type, Allocator& alloc)
{
  switch (type) {
  case AdapterType::DIRECT:
    return toJson(DIRECT_TYPE, alloc);
  case AdapterType::REJECT:
    return toJson(REJECT_TYPE, alloc);
  case AdapterType::SOCKS5:
    return toJson(SOCKS5_TYPE, alloc);
  case AdapterType::HTTP:
    return toJson(HTTP_TYPE, alloc);
  case AdapterType::SS:
    return toJson(SS_TYPE, alloc);
  default:
    fail(PichiError::MISC);
  }
}

json::Value toJson(CryptoMethod method, Allocator& alloc)
{
  switch (method) {
  case CryptoMethod::RC4_MD5:
    return toJson(RC4_MD5_METHOD, alloc);
  case CryptoMethod::BF_CFB:
    return toJson(BF_CFB_METHOD, alloc);
  case CryptoMethod::AES_128_CTR:
    return toJson(AES_128_CTR_METHOD, alloc);
  case CryptoMethod::AES_192_CTR:
    return toJson(AES_192_CTR_METHOD, alloc);
  case CryptoMethod::AES_256_CTR:
    return toJson(AES_256_CTR_METHOD, alloc);
  case CryptoMethod::AES_128_CFB:
    return toJson(AES_128_CFB_METHOD, alloc);
  case CryptoMethod::AES_192_CFB:
    return toJson(AES_192_CFB_METHOD, alloc);
  case CryptoMethod::AES_256_CFB:
    return toJson(AES_256_CFB_METHOD, alloc);
  case CryptoMethod::CAMELLIA_128_CFB:
    return toJson(CAMELLIA_128_CFB_METHOD, alloc);
  case CryptoMethod::CAMELLIA_192_CFB:
    return toJson(CAMELLIA_192_CFB_METHOD, alloc);
  case CryptoMethod::CAMELLIA_256_CFB:
    return toJson(CAMELLIA_256_CFB_METHOD, alloc);
  case CryptoMethod::CHACHA20:
    return toJson(CHACHA20_METHOD, alloc);
  case CryptoMethod::SALSA20:
    return toJson(SALSA20_METHOD, alloc);
  case CryptoMethod::CHACHA20_IETF:
    return toJson(CHACHA20_IETF_METHOD, alloc);
  case CryptoMethod::AES_128_GCM:
    return toJson(AES_128_GCM_METHOD, alloc);
  case CryptoMethod::AES_192_GCM:
    return toJson(AES_192_GCM_METHOD, alloc);
  case CryptoMethod::AES_256_GCM:
    return toJson(AES_256_GCM_METHOD, alloc);
  case CryptoMethod::CHACHA20_IETF_POLY1305:
    return toJson(CHACHA20_IETF_POLY1305_METHOD, alloc);
  case CryptoMethod::XCHACHA20_IETF_POLY1305:
    return toJson(XCHACHA20_IETF_POLY1305_METHOD, alloc);
  default:
    fail(PichiError::MISC);
  }
}

json::Value toJson(IngressVO const& ingress, Allocator& alloc)
{
  auto ret = json::Value{};
  ret.SetObject();
  switch (ingress.type_) {
  case AdapterType::SS:
    assertTrue(ingress.method_.has_value(), PichiError::MISC);
    assertTrue(ingress.password_.has_value(), PichiError::MISC);
    assertFalse(ingress.password_->empty(), PichiError::MISC);
    ret.AddMember(IngressVOKey::method_, toJson(*ingress.method_, alloc), alloc);
    ret.AddMember(IngressVOKey::password_, toJson(*ingress.password_, alloc), alloc);
    // Don't break here
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    assertFalse(ingress.bind_.empty(), PichiError::MISC);
    assertFalse(ingress.port_ == 0, PichiError::MISC);
    ret.AddMember(IngressVOKey::bind_, toJson(ingress.bind_, alloc), alloc);
    ret.AddMember(IngressVOKey::port_, json::Value{ingress.port_}, alloc);
  // Don't break here
  case AdapterType::DIRECT:
  case AdapterType::REJECT:
    ret.AddMember(IngressVOKey::type_, toJson(ingress.type_, alloc), alloc);
    break;
  default:
    fail(PichiError::MISC);
  }
  return ret;
}

json::Value toJson(OutboundVO const& ovo, Allocator& alloc)
{
  auto outbound = json::Value{};
  outbound.SetObject();

  switch (ovo.type_) {
  case AdapterType::SS:
    assertTrue(ovo.method_.has_value(), PichiError::MISC);
    assertTrue(ovo.password_.has_value(), PichiError::MISC);
    assertFalse(ovo.password_->empty(), PichiError::MISC);
    outbound.AddMember(OutboundVOKey::method_, toJson(*ovo.method_, alloc), alloc);
    outbound.AddMember(OutboundVOKey::password_, toJson(*ovo.password_, alloc), alloc);
    // Don't break here
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(ovo.host_.has_value(), PichiError::MISC);
    assertFalse(ovo.host_->empty(), PichiError::MISC);
    assertTrue(ovo.port_.has_value(), PichiError::MISC);
    assertFalse(*ovo.port_ == 0, PichiError::MISC);
    outbound.AddMember(OutboundVOKey::host_, toJson(*ovo.host_, alloc), alloc);
    outbound.AddMember(OutboundVOKey::port_, json::Value{*ovo.port_}, alloc);
    // Don't break here
  case AdapterType::DIRECT:
  case AdapterType::REJECT:
    outbound.AddMember(OutboundVOKey::type_, toJson(ovo.type_, alloc), alloc);
    break;
  default:
    fail(PichiError::MISC);
  }

  return outbound;
}

json::Value toJson(RuleVO const& rvo, Allocator& alloc)
{
  assertFalse(rvo.outbound_.empty(), PichiError::MISC);

  auto rule = json::Value{};
  rule.SetObject();
  rule.AddMember(RuleVOKey::outbound_, toJson(rvo.outbound_, alloc), alloc);
  if (!rvo.range_.empty())
    rule.AddMember(RuleVOKey::range_, toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    rule.AddMember(RuleVOKey::ingress_, toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc),
                   alloc);
  if (!rvo.type_.empty())
    rule.AddMember(RuleVOKey::type_, toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    rule.AddMember(RuleVOKey::pattern_, toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc),
                   alloc);
  if (!rvo.domain_.empty())
    rule.AddMember(RuleVOKey::domain_, toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    rule.AddMember(RuleVOKey::country_, toJson(begin(rvo.country_), end(rvo.country_), alloc),
                   alloc);
  return rule;
}

json::Value toJson(RouteVO const& rvo, Allocator& alloc)
{
  // "default" is required when sending to clients
  assertTrue(rvo.default_.has_value(), PichiError::MISC);
  assertFalse(rvo.default_->empty(), PichiError::MISC);

  auto route = json::Value{};
  route.SetObject();
  route.AddMember(RouteVOKey::default_, toJson(*rvo.default_, alloc), alloc);
  route.AddMember(RouteVOKey::rules_, toJson(begin(rvo.rules_), end(rvo.rules_), alloc), alloc);
  return route;
}

template <> IngressVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::MISC);
  assertTrue(v.HasMember(IngressVOKey::type_), PichiError::MISC);

  auto ingress = IngressVO{};

  ingress.type_ = parseAdapterType(v[IngressVOKey::type_]);

  switch (ingress.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(IngressVOKey::method_), PichiError::MISC);
    assertTrue(v.HasMember(IngressVOKey::password_), PichiError::MISC);
    ingress.method_ = parseCryptoMethod(v[IngressVOKey::method_]);
    ingress.password_ = parseString(v[IngressVOKey::password_]);
    // Don't break here
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(v.HasMember(IngressVOKey::bind_), PichiError::MISC);
    assertTrue(v.HasMember(IngressVOKey::port_), PichiError::MISC);
    ingress.bind_ = parseString(v[IngressVOKey::bind_]);
    ingress.port_ = parsePort(v[IngressVOKey::port_]);
    // Don't break here
  case AdapterType::DIRECT:
  case AdapterType::REJECT:
    break;
  default:
    fail(PichiError::MISC);
  }

  return ingress;
}

template <> OutboundVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::MISC);
  assertTrue(v.HasMember(OutboundVOKey::type_), PichiError::MISC);

  auto ovo = OutboundVO{};
  ovo.type_ = parseAdapterType(v[OutboundVOKey::type_]);

  switch (ovo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(OutboundVOKey::method_), PichiError::MISC);
    assertTrue(v.HasMember(OutboundVOKey::password_), PichiError::MISC);
    ovo.method_ = parseCryptoMethod(v[OutboundVOKey::method_]);
    ovo.password_ = parseString(v[OutboundVOKey::password_]);
    // Don't break here
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(v.HasMember(OutboundVOKey::host_), PichiError::MISC);
    assertTrue(v.HasMember(OutboundVOKey::port_), PichiError::MISC);
    ovo.host_ = parseString(v[OutboundVOKey::host_]);
    ovo.port_ = parsePort(v[OutboundVOKey::port_]);
    // Don't break here
  case AdapterType::DIRECT:
  case AdapterType::REJECT:
    break;
  default:
    fail(PichiError::MISC);
  }

  return ovo;
}

template <> RuleVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::MISC);
  assertTrue(v.HasMember(RuleVOKey::outbound_), PichiError::MISC);

  auto rvo = RuleVO{};

  rvo.outbound_ = parseString(v[RuleVOKey::outbound_]);

  parseArray(v, RuleVOKey::range_, back_inserter(rvo.range_), &parseString);
  parseArray(v, RuleVOKey::ingress_, back_inserter(rvo.ingress_), &parseString);
  parseArray(v, RuleVOKey::type_, back_inserter(rvo.type_), &parseAdapterType);
  parseArray(v, RuleVOKey::pattern_, back_inserter(rvo.pattern_), &parseString);
  parseArray(v, RuleVOKey::domain_, back_inserter(rvo.domain_), &parseString);
  parseArray(v, RuleVOKey::country_, back_inserter(rvo.country_), &parseString);

  return rvo;
}

template <> RouteVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::MISC);

  auto rvo = RouteVO{};

  if (v.HasMember(RouteVOKey::default_)) rvo.default_.emplace(parseString(v[RouteVOKey::default_]));

  parseArray(v, RouteVOKey::rules_, back_inserter(rvo.rules_), &parseString);

  return rvo;
}

} // namespace api
} // namespace pichi
