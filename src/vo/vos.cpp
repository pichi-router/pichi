#include <numeric>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <pichi/vo/vos.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(IngressVO const& ingress, Allocator& alloc)
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

json::Value toJson(EgressVO const& evo, Allocator& alloc)
{
  auto egress_ = json::Value{};
  egress_.SetObject();
  egress_.AddMember(egress::TYPE, toJson(evo.type_, alloc), alloc);
  if (evo.type_ != AdapterType::DIRECT && evo.type_ != AdapterType::REJECT) {
    assertTrue(evo.host_.has_value(), PichiError::MISC);
    assertFalse(evo.host_->empty(), PichiError::MISC);
    assertTrue(evo.port_.has_value(), PichiError::MISC);
    assertFalse(*evo.port_ == 0_u16, PichiError::MISC);
    egress_.AddMember(egress::HOST, toJson(*evo.host_, alloc), alloc);
    egress_.AddMember(egress::PORT, json::Value{*evo.port_}, alloc);
  }
  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(evo.method_.has_value(), PichiError::MISC);
    assertTrue(evo.password_.has_value(), PichiError::MISC);
    assertFalse(evo.password_->empty(), PichiError::MISC);
    egress_.AddMember(egress::METHOD, toJson(*evo.method_, alloc), alloc);
    egress_.AddMember(egress::PASSWORD, toJson(*evo.password_, alloc), alloc);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    assertTrue(evo.tls_.has_value(), PichiError::MISC);
    egress_.AddMember(egress::TLS, *evo.tls_, alloc);
    if (*evo.tls_) {
      assertTrue(evo.insecure_.has_value(), PichiError::MISC);
      egress_.AddMember(egress::INSECURE, *evo.insecure_, alloc);
      if (!*evo.insecure_ && evo.caFile_.has_value()) {
        assertFalse(evo.caFile_->empty());
        egress_.AddMember(egress::CA_FILE, toJson(*evo.caFile_, alloc), alloc);
      }
    }
    break;
  case AdapterType::REJECT:
    assertTrue(evo.mode_.has_value());
    egress_.AddMember(egress::MODE, toJson(*evo.mode_, alloc), alloc);
    if (*evo.mode_ == DelayMode::FIXED) {
      assertTrue(evo.delay_.has_value());
      assertTrue(*evo.delay_ <= 300_u16);
      egress_.AddMember(egress::DELAY, json::Value{*evo.delay_}, alloc);
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
    rule.AddMember(rule::RANGE, toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    rule.AddMember(rule::INGRESS, toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    rule.AddMember(rule::TYPE, toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    rule.AddMember(rule::PATTERN, toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    rule.AddMember(rule::DOMAIN_NAME, toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    rule.AddMember(rule::COUNTRY, toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);
  return rule;
}

json::Value toJson(RouteVO const& rvo, Allocator& alloc)
{
  // "default" is required when sending to clients
  assertTrue(rvo.default_.has_value(), PichiError::MISC);
  assertFalse(rvo.default_->empty(), PichiError::MISC);

  auto route = json::Value{};
  route.SetObject();
  route.AddMember(route::DEFAULT, toJson(*rvo.default_, alloc), alloc);

  auto rules = json::Value{};
  rules.SetArray();
  for_each(cbegin(rvo.rules_), cend(rvo.rules_), [&](auto&& item) {
    auto rule = json::Value{};
    rule.SetArray();
    for_each(cbegin(item.first), cend(item.first),
             [&](auto&& name) { rule.PushBack(toJson(name, alloc), alloc); });
    rule.PushBack(toJson(item.second, alloc), alloc);
    rules.PushBack(move(rule), alloc);
  });
  route.AddMember(route::RULES, move(rules), alloc);
  return route;
}

json::Value toJson(ErrorVO const& evo, Allocator& alloc)
{
  using StringRef = json::Value::StringRefType;
  using SizeType = json::SizeType;

  auto error = json::Value{};
  error.SetObject();
  error.AddMember(error::MESSAGE,
                  StringRef{evo.message_.data(), static_cast<SizeType>(evo.message_.size())},
                  alloc);
  return error;
}

template <> IngressVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(ingress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto ivo = IngressVO{};

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

template <> EgressVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);
  assertTrue(v.HasMember(egress::TYPE), PichiError::BAD_JSON, msg::MISSING_TYPE_FIELD);

  auto evo = EgressVO{};
  evo.type_ = parse<AdapterType>(v[egress::TYPE]);

  if (evo.type_ == AdapterType::HTTP || evo.type_ == AdapterType::SOCKS5 ||
      evo.type_ == AdapterType::SS) {
    assertTrue(v.HasMember(egress::HOST), PichiError::BAD_JSON, msg::MISSING_HOST_FIELD);
    assertTrue(v.HasMember(egress::PORT), PichiError::BAD_JSON, msg::MISSING_PORT_FIELD);
    evo.host_ = parse<string>(v[egress::HOST]);
    evo.port_ = parsePort(v[egress::PORT]);
  }

  switch (evo.type_) {
  case AdapterType::SS:
    assertTrue(v.HasMember(egress::METHOD), PichiError::BAD_JSON, msg::MISSING_METHOD_FIELD);
    assertTrue(v.HasMember(egress::PASSWORD), PichiError::BAD_JSON, msg::MISSING_PW_FIELD);
    evo.method_ = parse<CryptoMethod>(v[egress::METHOD]);
    evo.password_ = parse<string>(v[egress::PASSWORD]);
    break;
  case AdapterType::SOCKS5:
  case AdapterType::HTTP:
    evo.tls_ = v.HasMember(egress::TLS) && parse<bool>(v[egress::TLS]);
    if (*evo.tls_) {
      evo.insecure_ = v.HasMember(egress::INSECURE) && parse<bool>(v[egress::INSECURE]);
      if (!*evo.insecure_ && v.HasMember(egress::CA_FILE))
        evo.caFile_ = parse<string>(v[egress::CA_FILE]);
    }
    if (v.HasMember(egress::CREDENTIAL)) {
      evo.credential_ = parsePair(v[egress::CREDENTIAL], parseNameOrPassword);
    }
    break;
  case AdapterType::REJECT:
    if (v.HasMember(egress::MODE)) {
      evo.mode_ = parse<DelayMode>(v[egress::MODE]);
      if (evo.mode_ == DelayMode::FIXED) {
        assertTrue(v.HasMember(egress::DELAY), PichiError::BAD_JSON, msg::MISSING_DELAY_FIELD);
        assertTrue(v[egress::DELAY].IsInt(), PichiError::BAD_JSON, msg::INT_TYPE_ERROR);
        auto delay = v[egress::DELAY].GetInt();
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
  auto parseString = [](auto&& v) { return parse<string>(v); };
  auto parseAdapterType = [](auto&& v) { return parse<AdapterType>(v); };

  parseArray(v, rule::RANGE, back_inserter(rvo.range_), parseString);
  parseArray(v, rule::INGRESS, back_inserter(rvo.ingress_), parseString);
  parseArray(v, rule::TYPE, back_inserter(rvo.type_), parseAdapterType);
  parseArray(v, rule::PATTERN, back_inserter(rvo.pattern_), parseString);
  parseArray(v, rule::DOMAIN_NAME, back_inserter(rvo.domain_), parseString);
  parseArray(v, rule::COUNTRY, back_inserter(rvo.country_), parseString);

  return rvo;
}

template <> RouteVO parse(json::Value const& v)
{
  assertTrue(v.IsObject(), PichiError::BAD_JSON, msg::OBJ_TYPE_ERROR);

  auto rvo = RouteVO{};

  if (v.HasMember(route::DEFAULT)) rvo.default_.emplace(parse<string>(v[route::DEFAULT]));

  parseArray(v, route::RULES, back_inserter(rvo.rules_), [](auto&& v) {
    assertTrue(v.IsArray(), PichiError::BAD_JSON, msg::ARY_TYPE_ERROR);
    auto array = v.GetArray();
    assertTrue(array.Size() >= 2, PichiError::BAD_JSON, msg::ARY_SIZE_ERROR);
    auto first = array.Begin();
    auto last = first + (array.Size() - 1);
    return make_pair(accumulate(first, last, vector<string>{},
                                [](auto&& full, auto&& item) {
                                  full.push_back(parse<string>(item));
                                  return move(full);
                                }),
                     parse<string>(*last));
  });

  return rvo;
}

}  // namespace pichi::vo
