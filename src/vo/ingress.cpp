#include <numeric>
#include <pichi/common/literals.hpp>
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
  auto ret = json::Value{};
  ret.SetObject();
  if (ingress.type_ == AdapterType::HTTP || ingress.type_ == AdapterType::SOCKS5 ||
      ingress.type_ == AdapterType::SS || ingress.type_ == AdapterType::TUNNEL) {
    assertFalse(ingress.bind_.empty(), PichiError::MISC);
    assertFalse(ingress.port_ == 0_u16, PichiError::MISC);
    ret.AddMember(ingress::BIND, toJson(ingress.bind_, alloc), alloc);
    ret.AddMember(ingress::PORT, json::Value{ingress.port_}, alloc);
    ret.AddMember(ingress::TYPE, toJson(ingress.type_, alloc), alloc);
  }
  switch (ingress.type_) {
  case AdapterType::SS:
    assertTrue(ingress.method_.has_value(), PichiError::MISC);
    assertTrue(ingress.password_.has_value(), PichiError::MISC);
    assertFalse(ingress.password_->empty(), PichiError::MISC);
    ret.AddMember(ingress::METHOD, toJson(*ingress.method_, alloc), alloc);
    ret.AddMember(ingress::PASSWORD, toJson(*ingress.password_, alloc), alloc);
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    assertTrue(ingress.tls_.has_value(), PichiError::MISC);
    ret.AddMember(ingress::TLS, *ingress.tls_, alloc);
    if (*ingress.tls_) {
      assertTrue(ingress.certFile_.has_value(), PichiError::MISC);
      assertFalse(ingress.certFile_->empty(), PichiError::MISC);
      assertTrue(ingress.keyFile_.has_value(), PichiError::MISC);
      assertFalse(ingress.keyFile_->empty(), PichiError::MISC);
      ret.AddMember(ingress::CERT_FILE, toJson(*ingress.certFile_, alloc), alloc);
      ret.AddMember(ingress::KEY_FILE, toJson(*ingress.keyFile_, alloc), alloc);
    }
    if (!ingress.credentials_.empty()) {
      ret.AddMember(ingress::CREDENTIALS,
                    accumulate(cbegin(ingress.credentials_), cend(ingress.credentials_),
                               move(json::Value{}.SetObject()),
                               [&alloc](auto&& s, auto&& i) {
                                 assertTrue(i.first.size() < 256, msg::TOO_LONG_NAME_PASSWORD);
                                 assertTrue(i.second.size() < 256, msg::TOO_LONG_NAME_PASSWORD);
                                 s.AddMember(toJson(i.first, alloc), toJson(i.second, alloc),
                                             alloc);
                                 return move(s);
                               }),
                    alloc);
    }
    break;
  case AdapterType::TUNNEL:
    assertFalse(ingress.destinations_.empty());
    assertTrue(ingress.balance_.has_value());
    ret.AddMember(ingress::DESTINATIONS,
                  toJson(cbegin(ingress.destinations_), cend(ingress.destinations_), alloc), alloc);
    ret.AddMember(ingress::BALANCE, toJson(*ingress.balance_, alloc), alloc);
    break;
  default:
    fail(PichiError::MISC);
  }
  return ret;
}

template <> Ingress parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(ingress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto ivo = Ingress{};

  ivo.type_ = parse<AdapterType>(v[ingress::TYPE]);
  if (ivo.type_ == AdapterType::HTTP || ivo.type_ == AdapterType::SOCKS5 ||
      ivo.type_ == AdapterType::SS || ivo.type_ == AdapterType::TUNNEL) {
    assertTrue(v.HasMember(ingress::BIND), PichiError::BAD_JSON, msg::MISSING_BIND_FIELD);
    assertTrue(v.HasMember(ingress::PORT), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
    ivo.bind_ = parse<string>(v[ingress::BIND]);
    ivo.port_ = parsePort(v[ingress::PORT]);
  }

  switch (ivo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(ingress::METHOD), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
    assertTrue(v.HasMember(ingress::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    ivo.method_ = parse<CryptoMethod>(v[ingress::METHOD]);
    ivo.password_ = parse<string>(v[ingress::PASSWORD]);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    ivo.tls_ = v.HasMember(ingress::TLS) && parse<bool>(v[ingress::TLS]);
    if (*ivo.tls_) {
      assertTrue(v.HasMember(ingress::CERT_FILE), PichiError::BAD_JSON,
                 msg::MISSING_CERT_FILE_FIELD);
      assertTrue(v.HasMember(ingress::KEY_FILE), PichiError::BAD_JSON, msg::MISSING_KEY_FILE_FIELD);
      ivo.certFile_ = parse<string>(v[ingress::CERT_FILE]);
      ivo.keyFile_ = parse<string>(v[ingress::KEY_FILE]);
    }
    if (v.HasMember(ingress::CREDENTIALS)) {
      auto& credentials = v[ingress::CREDENTIALS];
      assertTrue(credentials.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
      assertFalse(credentials.MemberCount() == 0, PichiError::BAD_JSON, msg::CRE_EMPTY);
      ivo.credentials_ =
          accumulate(credentials.MemberBegin(), credentials.MemberEnd(),
                     unordered_map<string, string>{}, [](auto&& credentials, auto&& credential) {
                       credentials.emplace(parseNameOrPassword(credential.name),
                                           parseNameOrPassword(credential.value));
                       return move(credentials);
                     });
    }
    break;
  case AdapterType::TUNNEL:
    assertTrue(v.HasMember(ingress::DESTINATIONS), PichiError::BAD_JSON,
               msg::MISSING_DESTINATIONS_FIELD);
    assertTrue(v.HasMember(ingress::BALANCE), PichiError::BAD_JSON, msg::MISSING_BALANCE_FIELD);
    ivo.destinations_ = parseDestinantions(v[ingress::DESTINATIONS]);
    ivo.balance_ = parse<BalanceType>(v[ingress::BALANCE]);
    break;
  default:
    fail(PichiError::BAD_JSON, msg::AT_INVALID);
  }

  return ivo;
}

}  // namespace pichi::vo
