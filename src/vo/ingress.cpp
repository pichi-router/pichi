#include <algorithm>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(Ingress const& ingress, Allocator& alloc)
{
  assertFalse(ingress.type_ == AdapterType::DIRECT);
  assertFalse(ingress.type_ == AdapterType::REJECT);

  auto ret = json::Value{json::kObjectType};

  ret.AddMember(ingress::TYPE, toJson(ingress.type_, alloc), alloc);

  assertFalse(ingress.bind_.empty());
  ret.AddMember(ingress::BIND, json::Value(json::kArrayType), alloc);
  for_each(cbegin(ingress.bind_), cend(ingress.bind_),
           [&bind = ret[ingress::BIND], &alloc](auto&& endpoint) {
             bind.PushBack(toJson(endpoint, alloc), alloc);
           });

  switch (ingress.type_) {
  case AdapterType::TUNNEL:
    assertTrue(ingress.opt_.has_value());
    ret.AddMember(ingress::OPTION, toJson(get<TunnelOption>(*ingress.opt_), alloc), alloc);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    if (ingress.tls_.has_value()) ret.AddMember(ingress::TLS, toJson(*ingress.tls_, alloc), alloc);
    if (ingress.credential_.has_value())
      ret.AddMember(ingress::CREDENTIAL,
                    toJson(get<up::IngressCredential>(*ingress.credential_), alloc), alloc);
    break;
  case AdapterType::SS:
    assertTrue(ingress.opt_.has_value());
    ret.AddMember(ingress::OPTION, toJson(get<ShadowsocksOption>(*ingress.opt_), alloc), alloc);
    break;
  case AdapterType::TROJAN:
    assertTrue(ingress.opt_.has_value());
    assertTrue(ingress.tls_.has_value());
    assertTrue(ingress.credential_.has_value());
    ret.AddMember(ingress::OPTION, toJson(get<TrojanOption>(*ingress.opt_), alloc), alloc);
    ret.AddMember(ingress::TLS, toJson(*ingress.tls_, alloc), alloc);
    ret.AddMember(ingress::CREDENTIAL,
                  toJson(get<trojan::IngressCredential>(*ingress.credential_), alloc), alloc);
    if (ingress.websocket_.has_value())
      ret.AddMember(ingress::WEBSOCKET, toJson(*ingress.websocket_, alloc), alloc);
    break;
  case AdapterType::VMESS:
    assertTrue(ingress.credential_.has_value());
    ret.AddMember(ingress::CREDENTIAL,
                  toJson(get<vmess::IngressCredential>(*ingress.credential_), alloc), alloc);
    if (ingress.tls_.has_value()) ret.AddMember(ingress::TLS, toJson(*ingress.tls_, alloc), alloc);
    if (ingress.websocket_.has_value())
      ret.AddMember(ingress::WEBSOCKET, toJson(*ingress.websocket_, alloc), alloc);
    break;
  default:
    fail();
  }
  return ret;
}

template <> Ingress parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto ingress = Ingress{};

  assertTrue(v.HasMember(ingress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);
  ingress.type_ = parse<AdapterType>(v[ingress::TYPE]);
  assertFalse(ingress.type_ == AdapterType::DIRECT, PichiError::BAD_JSON, msg::AT_INVALID);
  assertFalse(ingress.type_ == AdapterType::REJECT, PichiError::BAD_JSON, msg::AT_INVALID);

  assertTrue(v.HasMember(ingress::BIND), PichiError::BAD_JSON, msg::MISSING_BIND_FIELD);
  auto&& bind = v[ingress::BIND];
  assertTrue(bind.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
  assertFalse(bind.Empty(), PichiError::BAD_JSON, msg::ARY_SIZE_ERROR);
  transform(bind.Begin(), bind.End(), back_inserter(ingress.bind_),
            [](auto&& v) { return parse<Endpoint>(v); });

  switch (ingress.type_) {
  case AdapterType::TUNNEL:
    assertTrue(v.HasMember(ingress::OPTION), PichiError::BAD_JSON, msg::MISSING_OPTION_FIELD);
    ingress.opt_ = parse<TunnelOption>(v[ingress::OPTION]);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    if (v.HasMember(ingress::TLS)) ingress.tls_ = parse<TlsIngressOption>(v[ingress::TLS]);
    if (v.HasMember(ingress::CREDENTIAL))
      ingress.credential_ = parse<up::IngressCredential>(v[ingress::CREDENTIAL]);
    break;
  case AdapterType::SS:
    assertTrue(v.HasMember(ingress::OPTION), PichiError::BAD_JSON, msg::MISSING_OPTION_FIELD);
    ingress.opt_ = parse<ShadowsocksOption>(v[ingress::OPTION]);
    break;
  case AdapterType::TROJAN:
    assertTrue(v.HasMember(ingress::CREDENTIAL), PichiError::BAD_JSON, msg::MISSING_CRED_FIELD);
    assertTrue(v.HasMember(ingress::OPTION), PichiError::BAD_JSON, msg::MISSING_OPTION_FIELD);
    assertTrue(v.HasMember(ingress::TLS), PichiError::BAD_JSON, msg::MISSING_TLS_FIELD);
    ingress.credential_ = parse<trojan::IngressCredential>(v[ingress::CREDENTIAL]);
    ingress.opt_ = parse<TrojanOption>(v[ingress::OPTION]);
    ingress.tls_ = parse<TlsIngressOption>(v[ingress::TLS]);
    if (v.HasMember(ingress::WEBSOCKET))
      ingress.websocket_ = parse<WebsocketOption>(v[ingress::WEBSOCKET]);
    break;
  case AdapterType::VMESS:
    assertTrue(v.HasMember(ingress::CREDENTIAL), PichiError::BAD_JSON, msg::MISSING_CRED_FIELD);
    ingress.credential_ = parse<vmess::IngressCredential>(v[ingress::CREDENTIAL]);
    if (v.HasMember(ingress::TLS)) ingress.tls_ = parse<TlsIngressOption>(v[ingress::TLS]);
    if (v.HasMember(ingress::WEBSOCKET))
      ingress.websocket_ = parse<WebsocketOption>(v[ingress::WEBSOCKET]);
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
    break;
  }
  return ingress;
}

/*
 * TODO It's unknown that comparing 'optional<detail::Credential>'s will cause compilation errors
 *        if 'operator==' is directly used.
 */
template <typename Credential, typename Variant>
bool compare(optional<Variant> const& lhs, optional<Variant> const& rhs)
{
  if (lhs.has_value() && rhs.has_value())
    return get<Credential>(*lhs) == get<Credential>(*rhs);
  else
    return !lhs.has_value() && !rhs.has_value();
}

bool operator==(Ingress const& lhs, Ingress const& rhs)
{
  if (lhs.bind_ != rhs.bind_ || lhs.type_ != rhs.type_) return false;
  switch (lhs.type_) {
  case AdapterType::TUNNEL:
  case AdapterType::SS:
    return lhs.opt_ == rhs.opt_;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    return lhs.tls_ == rhs.tls_ && compare<up::IngressCredential>(lhs.credential_, rhs.credential_);
  case AdapterType::TROJAN:
    return compare<trojan::IngressCredential>(lhs.credential_, rhs.credential_) &&
           lhs.opt_ == rhs.opt_ && lhs.tls_ == rhs.tls_ && lhs.websocket_ == rhs.websocket_;
  case AdapterType::VMESS:
    return compare<vmess::IngressCredential>(lhs.credential_, rhs.credential_) &&
           lhs.tls_ == rhs.tls_ && lhs.websocket_ == rhs.websocket_;
  default:
    fail();
  }
}

}  // namespace pichi::vo
