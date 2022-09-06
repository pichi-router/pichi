#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(Egress const& egress, Allocator& alloc)
{
  assertFalse(egress.type_ == AdapterType::TUNNEL);

  auto ret = json::Value{json::kObjectType};
  ret.AddMember(egress::TYPE, toJson(egress.type_, alloc), alloc);

  switch (egress.type_) {
  case AdapterType::DIRECT:
    break;
  case AdapterType::REJECT:
    assertTrue(egress.opt_.has_value());
    ret.AddMember(egress::OPTION, toJson(get<RejectOption>(*egress.opt_), alloc), alloc);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    assertTrue(egress.server_.has_value());
    ret.AddMember(egress::SERVER, toJson(*egress.server_, alloc), alloc);
    if (egress.credential_.has_value())
      ret.AddMember(egress::CREDENTIAL, toJson(get<UpEgressCredential>(*egress.credential_), alloc),
                    alloc);
    if (egress.tls_.has_value()) ret.AddMember(egress::TLS, toJson(*egress.tls_, alloc), alloc);
    break;
  case AdapterType::SS:
    assertTrue(egress.server_.has_value());
    ret.AddMember(egress::SERVER, toJson(*egress.server_, alloc), alloc);
    assertTrue(egress.opt_.has_value());
    ret.AddMember(egress::OPTION, toJson(get<ShadowsocksOption>(*egress.opt_), alloc), alloc);
    break;
  case AdapterType::TROJAN:
    assertTrue(egress.server_.has_value());
    assertTrue(egress.credential_.has_value());
    assertTrue(egress.tls_.has_value());
    ret.AddMember(egress::SERVER, toJson(*egress.server_, alloc), alloc);
    ret.AddMember(egress::CREDENTIAL,
                  toJson(get<TrojanEgressCredential>(*egress.credential_), alloc), alloc);
    ret.AddMember(egress::TLS, toJson(*egress.tls_, alloc), alloc);
    if (egress.websocket_.has_value())
      ret.AddMember(egress::WEBSOCKET, toJson(*egress.websocket_, alloc), alloc);
    break;
  case AdapterType::VMESS:
    assertTrue(egress.server_.has_value());
    assertTrue(egress.credential_.has_value());
    ret.AddMember(egress::SERVER, toJson(*egress.server_, alloc), alloc);
    ret.AddMember(egress::CREDENTIAL,
                  toJson(get<VMessEgressCredential>(*egress.credential_), alloc), alloc);
    if (egress.tls_.has_value()) ret.AddMember(egress::TLS, toJson(*egress.tls_, alloc), alloc);
    if (egress.websocket_.has_value())
      ret.AddMember(egress::WEBSOCKET, toJson(*egress.websocket_, alloc), alloc);
    break;
  default:
    fail();
  }
  return ret;
}

template <> Egress parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(egress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto egress = Egress{};
  egress.type_ = parse<AdapterType>(v[egress::TYPE]);

  switch (egress.type_) {
  case AdapterType::DIRECT:
    break;
  case AdapterType::REJECT:
    assertTrue(v.HasMember(egress::OPTION), PichiError::BAD_JSON, msg::MISSING_OPTION_FIELD);
    egress.opt_ = parse<RejectOption>(v[egress::OPTION]);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    assertTrue(v.HasMember(egress::SERVER), PichiError::BAD_JSON, msg::MISSING_SERVER_FIELD);
    egress.server_ = parse<Endpoint>(v[egress::SERVER]);
    if (v.HasMember(egress::CREDENTIAL))
      egress.credential_ = parse<UpEgressCredential>(v[egress::CREDENTIAL]);
    if (v.HasMember(egress::TLS)) egress.tls_ = parse<TlsEgressOption>(v[egress::TLS]);
    break;
  case AdapterType::SS:
    assertTrue(v.HasMember(egress::SERVER), PichiError::BAD_JSON, msg::MISSING_SERVER_FIELD);
    assertTrue(v.HasMember(egress::OPTION), PichiError::BAD_JSON, msg::MISSING_OPTION_FIELD);
    egress.server_ = parse<Endpoint>(v[egress::SERVER]);
    egress.opt_ = parse<ShadowsocksOption>(v[egress::OPTION]);
    break;
  case AdapterType::TROJAN:
    assertTrue(v.HasMember(egress::SERVER), PichiError::BAD_JSON, msg::MISSING_SERVER_FIELD);
    assertTrue(v.HasMember(egress::CREDENTIAL), PichiError::BAD_JSON, msg::MISSING_CRED_FIELD);
    assertTrue(v.HasMember(egress::TLS), PichiError::BAD_JSON, msg::MISSING_TLS_FIELD);
    egress.server_ = parse<Endpoint>(v[egress::SERVER]);
    egress.credential_ = parse<TrojanEgressCredential>(v[egress::CREDENTIAL]);
    egress.tls_ = parse<TlsEgressOption>(v[egress::TLS]);
    if (v.HasMember(egress::WEBSOCKET))
      egress.websocket_ = parse<WebsocketOption>(v[egress::WEBSOCKET]);
    break;
  case AdapterType::VMESS:
    assertTrue(v.HasMember(egress::SERVER), PichiError::BAD_JSON, msg::MISSING_SERVER_FIELD);
    assertTrue(v.HasMember(egress::CREDENTIAL), PichiError::BAD_JSON, msg::MISSING_CRED_FIELD);
    egress.server_ = parse<Endpoint>(v[egress::SERVER]);
    egress.credential_ = parse<VMessEgressCredential>(v[egress::CREDENTIAL]);
    if (v.HasMember(egress::TLS)) egress.tls_ = parse<TlsEgressOption>(v[egress::TLS]);
    if (v.HasMember(egress::WEBSOCKET))
      egress.websocket_ = parse<WebsocketOption>(v[egress::WEBSOCKET]);
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
    break;
  }
  return egress;
}

bool operator==(Egress const& lhs, Egress const& rhs)
{
  if (lhs.type_ != rhs.type_) return false;
  switch (lhs.type_) {
  case AdapterType::DIRECT:
    return true;
  case AdapterType::REJECT:
    return lhs.opt_ == rhs.opt_;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    return lhs.server_ == rhs.server_ && lhs.tls_ == rhs.tls_ && lhs.credential_ == rhs.credential_;
  case AdapterType::SS:
    return lhs.server_ == rhs.server_ && lhs.opt_ == rhs.opt_;
  case AdapterType::TROJAN:
    return lhs.server_ == rhs.server_ && lhs.credential_ == rhs.credential_ &&
           lhs.tls_ == rhs.tls_ && lhs.websocket_ == rhs.websocket_;
  case AdapterType::VMESS:
    return lhs.server_ == rhs.server_ && lhs.credential_ == rhs.credential_ &&
           lhs.tls_ == rhs.tls_ && lhs.websocket_ == rhs.websocket_;
  default:
    fail();
  }
}

}  // namespace pichi::vo
