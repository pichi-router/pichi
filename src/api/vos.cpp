#include <limits>
#include <pichi/api/vos.hpp>
#include <pichi/common.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::api {

static decltype(auto) DIRECT_TYPE = "direct";
static decltype(auto) REJECT_TYPE = "reject";
static decltype(auto) SOCKS5_TYPE = "socks5";
static decltype(auto) HTTP_TYPE = "http";
static decltype(auto) SS_TYPE = "ss";
static decltype(auto) TUNNEL_TYPE = "tunnel";

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

static decltype(auto) RANDOM_DELAY_MODE = "random";
static decltype(auto) FIXED_DELAY_MODE = "fixed";

namespace IngressVOKey {

static decltype(auto) type_ = "type";
static decltype(auto) bind_ = "bind";
static decltype(auto) port_ = "port";
static decltype(auto) method_ = "method";
static decltype(auto) password_ = "password";
static decltype(auto) tls_ = "tls";
static decltype(auto) certFile_ = "cert_file";
static decltype(auto) keyFile_ = "key_file";
static decltype(auto) dstHost_ = "dst_host";
static decltype(auto) dstPort_ = "dst_port";

} // namespace IngressVOKey

namespace EgressVOKey {

static decltype(auto) type_ = "type";
static decltype(auto) host_ = "host";
static decltype(auto) port_ = "port";
static decltype(auto) method_ = "method";
static decltype(auto) password_ = "password";
static decltype(auto) mode_ = "mode";
static decltype(auto) delay_ = "delay";
static decltype(auto) tls_ = "tls";
static decltype(auto) insecure_ = "insecure";
static decltype(auto) caFile_ = "ca_file";

} // namespace EgressVOKey

namespace RuleVOKey {

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

namespace ErrorVOKey {

static decltype(auto) message_ = "message";

} // namespace ErrorVOKey

namespace msg {

static auto const OBJ_TYPE_ERROR = "JSON object required"sv;
static auto const ARY_TYPE_ERROR = "JSON array required"sv;
static auto const INT_TYPE_ERROR = "Integer required"sv;
static auto const STR_TYPE_ERROR = "String required"sv;
static auto const BOOL_TYPE_ERROR = "Boolean required"sv;
static auto const PAIR_TYPE_ERROR = "Pair required"sv;
static auto const AT_INVALID = "Invalid adapter type string"sv;
static auto const CM_INVALID = "Invalid crypto method string"sv;
static auto const PT_INVALID = "Port number must be in range (0, 65536)"sv;
static auto const DM_INVALID = "Invalid delay mode type string"sv;
static auto const DL_INVALID = "Delay time must be in range [0, 300]"sv;
static auto const STR_EMPTY = "Empty string"sv;
static auto const MISSING_TYPE_FIELD = "Missing type field"sv;
static auto const MISSING_HOST_FIELD = "Missing host field"sv;
static auto const MISSING_BIND_FIELD = "Missing bind field"sv;
static auto const MISSING_PORT_FIELD = "Missing port field"sv;
static auto const MISSING_METHOD_FIELD = "Missing method field"sv;
static auto const MISSING_PW_FIELD = "Missing password field"sv;
static auto const MISSING_DELAY_FIELD = "Missing delay field"sv;
static auto const MISSING_CERT_FILE_FIELD = "Missing cert_file field"sv;
static auto const MISSING_KEY_FILE_FIELD = "Missing key_file field"sv;
static auto const MISSING_DST_HOST_FIELD = "Missiong dst_host field"sv;
static auto const MISSING_DST_PORT_FIELD = "Missiong dst_port field"sv;

} // namespace msg

static DelayMode parseDelayMode(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == RANDOM_DELAY_MODE) return DelayMode::RANDOM;
  if (str == FIXED_DELAY_MODE) return DelayMode::FIXED;
  fail(PichiError::BAD_JSON, msg::DM_INVALID);
}

static AdapterType parseAdapterType(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto str = string_view{v.GetString()};
  if (str == DIRECT_TYPE) return AdapterType::DIRECT;
  if (str == REJECT_TYPE) return AdapterType::REJECT;
  if (str == SOCKS5_TYPE) return AdapterType::SOCKS5;
  if (str == HTTP_TYPE) return AdapterType::HTTP;
  if (str == SS_TYPE) return AdapterType::SS;
  if (str == TUNNEL_TYPE) return AdapterType::TUNNEL;
  fail(PichiError::BAD_JSON, msg::AT_INVALID);
}

static CryptoMethod parseCryptoMethod(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
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
  fail(PichiError::BAD_JSON, msg::CM_INVALID);
}

static uint16_t parsePort(json::Value const& v)
{
  assertTrue(v.IsInt(), PichiError::BAD_JSON, msg::INT_TYPE_ERROR);
  auto port = v.GetInt();
  assertTrue(port > 0_u16, PichiError::BAD_JSON, msg::PT_INVALID);
  assertTrue(port <= numeric_limits<uint16_t>::max(), PichiError::BAD_JSON, msg::PT_INVALID);
  return static_cast<uint16_t>(port);
}

static string parseString(json::Value const& v)
{
  assertTrue(v.IsString(), PichiError::BAD_JSON, msg::STR_TYPE_ERROR);
  auto ret = string{v.GetString()};
  assertFalse(ret.empty(), PichiError::BAD_JSON, msg::STR_EMPTY);
  return ret;
}

static auto parseBoolean(json::Value const& v)
{
  assertTrue(v.IsBool(), PichiError::BAD_JSON, msg::BOOL_TYPE_ERROR);
  return v.GetBool();
}

static pair<string, string> parseRule(json::Value const& v)
{
  assertTrue(v.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  auto array = v.GetArray();
  assertTrue(array.Size() == 2, PichiError::BAD_JSON, msg::PAIR_TYPE_ERROR);
  return make_pair(parseString(array[0]), parseString(array[1]));
}

template <typename OutputIt, typename T, typename Convert>
void parseArray(json::Value const& root, T const& key, OutputIt out, Convert&& convert)
{
  if (!root.HasMember(key)) return;
  assertTrue(root[key].IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  auto array = root[key].GetArray();
  transform(begin(array), end(array), out, forward<Convert>(convert));
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
    return toJson(FIXED_DELAY_MODE, alloc);
  case DelayMode::RANDOM:
    return toJson(RANDOM_DELAY_MODE, alloc);
  default:
    fail();
  }
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
  if (ingress.type_ == AdapterType::HTTP || ingress.type_ == AdapterType::SOCKS5 ||
      ingress.type_ == AdapterType::SS || ingress.type_ == AdapterType::TUNNEL) {
    assertFalse(ingress.bind_.empty(), PichiError::MISC);
    assertFalse(ingress.port_ == 0_u16, PichiError::MISC);
    ret.AddMember(IngressVOKey::bind_, toJson(ingress.bind_, alloc), alloc);
    ret.AddMember(IngressVOKey::port_, json::Value{ingress.port_}, alloc);
    ret.AddMember(IngressVOKey::type_, toJson(ingress.type_, alloc), alloc);
  }
  switch (ingress.type_) {
  case AdapterType::SS:
    assertTrue(ingress.method_.has_value(), PichiError::MISC);
    assertTrue(ingress.password_.has_value(), PichiError::MISC);
    assertFalse(ingress.password_->empty(), PichiError::MISC);
    ret.AddMember(IngressVOKey::method_, toJson(*ingress.method_, alloc), alloc);
    ret.AddMember(IngressVOKey::password_, toJson(*ingress.password_, alloc), alloc);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    assertTrue(ingress.tls_.has_value(), PichiError::MISC);
    ret.AddMember(IngressVOKey::tls_, *ingress.tls_, alloc);
    if (*ingress.tls_) {
      assertTrue(ingress.certFile_.has_value(), PichiError::MISC);
      assertFalse(ingress.certFile_->empty(), PichiError::MISC);
      assertTrue(ingress.keyFile_.has_value(), PichiError::MISC);
      assertFalse(ingress.keyFile_->empty(), PichiError::MISC);
      ret.AddMember(IngressVOKey::certFile_, toJson(*ingress.certFile_, alloc), alloc);
      ret.AddMember(IngressVOKey::keyFile_, toJson(*ingress.keyFile_, alloc), alloc);
    }
    break;
  case AdapterType::TUNNEL:
    assertTrue(ingress.dstHost_.has_value());
    assertTrue(ingress.dstPort_.has_value());
    assertFalse(*ingress.dstPort_ == 0_u16);
    ret.AddMember(IngressVOKey::dstHost_, toJson(*ingress.dstHost_, alloc), alloc);
    ret.AddMember(IngressVOKey::dstPort_, json::Value{*ingress.dstPort_}, alloc);
    break;
  default:
    fail(PichiError::MISC);
  }
  return ret;
}

json::Value toJson(EgressVO const& evo, Allocator& alloc)
{
  auto egress_ = json::Value{};
  egress_.SetObject();
  egress_.AddMember(EgressVOKey::type_, toJson(evo.type_, alloc), alloc);
  if (evo.type_ != AdapterType::DIRECT && evo.type_ != AdapterType::REJECT) {
    assertTrue(evo.host_.has_value(), PichiError::MISC);
    assertFalse(evo.host_->empty(), PichiError::MISC);
    assertTrue(evo.port_.has_value(), PichiError::MISC);
    assertFalse(*evo.port_ == 0_u16, PichiError::MISC);
    egress_.AddMember(EgressVOKey::host_, toJson(*evo.host_, alloc), alloc);
    egress_.AddMember(EgressVOKey::port_, json::Value{*evo.port_}, alloc);
  }
  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(evo.method_.has_value(), PichiError::MISC);
    assertTrue(evo.password_.has_value(), PichiError::MISC);
    assertFalse(evo.password_->empty(), PichiError::MISC);
    egress_.AddMember(EgressVOKey::method_, toJson(*evo.method_, alloc), alloc);
    egress_.AddMember(EgressVOKey::password_, toJson(*evo.password_, alloc), alloc);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(evo.tls_.has_value(), PichiError::MISC);
    egress_.AddMember(EgressVOKey::tls_, *evo.tls_, alloc);
    if (*evo.tls_) {
      assertTrue(evo.insecure_.has_value(), PichiError::MISC);
      egress_.AddMember(EgressVOKey::insecure_, *evo.insecure_, alloc);
      if (!*evo.insecure_ && evo.caFile_.has_value()) {
        assertFalse(evo.caFile_->empty());
        egress_.AddMember(EgressVOKey::caFile_, toJson(*evo.caFile_, alloc), alloc);
      }
    }
    break;
  case AdapterType::REJECT:
    assertTrue(evo.mode_.has_value());
    egress_.AddMember(EgressVOKey::mode_, toJson(*evo.mode_, alloc), alloc);
    if (*evo.mode_ == DelayMode::FIXED) {
      assertTrue(evo.delay_.has_value());
      assertTrue(*evo.delay_ <= 300_u16);
      egress_.AddMember(EgressVOKey::delay_, json::Value{*evo.delay_}, alloc);
    }
    break;
  case AdapterType::DIRECT:
    break;
  default:
    fail(PichiError::MISC);
  }

  return egress_;
}

json::Value toJson(RuleVO const& rvo, Allocator& alloc)
{
  auto rule = json::Value{};
  rule.SetObject();
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

  auto rules = json::Value{};
  rules.SetArray();
  for_each(cbegin(rvo.rules_), cend(rvo.rules_), [&](auto&& item) {
    auto rule = json::Value{};
    rule.SetArray();
    rule.PushBack(toJson(item.first, alloc), alloc);
    rule.PushBack(toJson(item.second, alloc), alloc);
    rules.PushBack(move(rule), alloc);
  });
  route.AddMember(RouteVOKey::rules_, move(rules), alloc);
  return route;
}

json::Value toJson(ErrorVO const& evo, Allocator& alloc)
{
  using StringRef = json::Value::StringRefType;
  using SizeType = json::SizeType;

  auto error = json::Value{};
  error.SetObject();
  error.AddMember(ErrorVOKey::message_,
                  StringRef{evo.message_.data(), static_cast<SizeType>(evo.message_.size())},
                  alloc);
  return error;
}

template <> IngressVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(IngressVOKey::type_), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto ivo = IngressVO{};

  ivo.type_ = parseAdapterType(v[IngressVOKey::type_]);
  if (ivo.type_ == AdapterType::HTTP || ivo.type_ == AdapterType::SOCKS5 ||
      ivo.type_ == AdapterType::SS || ivo.type_ == AdapterType::TUNNEL) {
    assertTrue(v.HasMember(IngressVOKey::bind_), PichiError::BAD_JSON, msg::MISSING_BIND_FIELD);
    assertTrue(v.HasMember(IngressVOKey::port_), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
    ivo.bind_ = parseString(v[IngressVOKey::bind_]);
    ivo.port_ = parsePort(v[IngressVOKey::port_]);
  }

  switch (ivo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(IngressVOKey::method_), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
    assertTrue(v.HasMember(IngressVOKey::password_), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    ivo.method_ = parseCryptoMethod(v[IngressVOKey::method_]);
    ivo.password_ = parseString(v[IngressVOKey::password_]);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    ivo.tls_ = v.HasMember(IngressVOKey::tls_) && parseBoolean(v[IngressVOKey::tls_]);
    if (*ivo.tls_) {
      assertTrue(v.HasMember(IngressVOKey::certFile_), PichiError::BAD_JSON,
                 msg::MISSING_CERT_FILE_FIELD);
      assertTrue(v.HasMember(IngressVOKey::keyFile_), PichiError::BAD_JSON,
                 msg::MISSING_KEY_FILE_FIELD);
      ivo.certFile_ = parseString(v[IngressVOKey::certFile_]);
      ivo.keyFile_ = parseString(v[IngressVOKey::keyFile_]);
    }
    break;
  case AdapterType::TUNNEL:
    assertTrue(v.HasMember(IngressVOKey::dstHost_), PichiError::BAD_JSON,
               msg::MISSING_DST_HOST_FIELD);
    assertTrue(v.HasMember(IngressVOKey::dstPort_), PichiError::BAD_JSON,
               msg::MISSING_DST_PORT_FIELD);
    ivo.dstHost_ = parseString(v[IngressVOKey::dstHost_]);
    ivo.dstPort_ = parsePort(v[IngressVOKey::dstPort_]);
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
  }

  return ivo;
}

template <> EgressVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(EgressVOKey::type_), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto evo = EgressVO{};
  evo.type_ = parseAdapterType(v[EgressVOKey::type_]);

  if (evo.type_ == AdapterType::HTTP || evo.type_ == AdapterType::SOCKS5 ||
      evo.type_ == AdapterType::SS) {
    assertTrue(v.HasMember(EgressVOKey::host_), PichiError::BAD_JSON, msg::MISSING_HOST_FIELD);
    assertTrue(v.HasMember(EgressVOKey::port_), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
    evo.host_ = parseString(v[EgressVOKey::host_]);
    evo.port_ = parsePort(v[EgressVOKey::port_]);
  }

  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(EgressVOKey::method_), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
    assertTrue(v.HasMember(EgressVOKey::password_), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    evo.method_ = parseCryptoMethod(v[EgressVOKey::method_]);
    evo.password_ = parseString(v[EgressVOKey::password_]);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    evo.tls_ = v.HasMember(EgressVOKey::tls_) && parseBoolean(v[EgressVOKey::tls_]);
    if (*evo.tls_) {
      evo.insecure_ =
          v.HasMember(EgressVOKey::insecure_) && parseBoolean(v[EgressVOKey::insecure_]);
      if (!*evo.insecure_ && v.HasMember(EgressVOKey::caFile_))
        evo.caFile_ = parseString(v[EgressVOKey::caFile_]);
    }
    break;
  case AdapterType::REJECT:
    if (v.HasMember(EgressVOKey::mode_)) {
      evo.mode_ = parseDelayMode(v[EgressVOKey::mode_]);
      if (evo.mode_ == DelayMode::FIXED) {
        assertTrue(v.HasMember(EgressVOKey::delay_), PichiError::BAD_JSON,
                   msg::MISSING_DELAY_FIELD);
        assertTrue(v[EgressVOKey::delay_].IsInt(), PichiError::BAD_JSON, msg::INT_TYPE_ERROR);
        auto delay = v[EgressVOKey::delay_].GetInt();
        assertTrue(delay >= 0_u16 && delay <= 300_u16, PichiError::BAD_JSON, msg::DL_INVALID);
        evo.delay_ = static_cast<uint16_t>(delay);
      }
    }
    else {
      evo.mode_ = DelayMode::FIXED;
      evo.delay_ = 0_u16;
    }
    break;
  case AdapterType::DIRECT:
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
  }

  return evo;
}

template <> RuleVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = RuleVO{};

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
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = RouteVO{};

  if (v.HasMember(RouteVOKey::default_)) rvo.default_.emplace(parseString(v[RouteVOKey::default_]));

  parseArray(v, RouteVOKey::rules_, back_inserter(rvo.rules_), &parseRule);

  return rvo;
}

} // namespace pichi::api
