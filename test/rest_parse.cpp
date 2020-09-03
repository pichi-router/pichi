#define BOOST_TEST_MODULE pichi rest_parse test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/common/exception.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string_view>

using namespace std;
using namespace rapidjson;
using namespace pichi;

using vo::parse;
using vo::toJson;

using pichi::AdapterType;

static string toString(Value const& v)
{
  auto buf = StringBuffer{};
  auto writer = Writer<StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

static string toString(vo::Ingress const& ingress)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember("type", toJson(ingress.type_, alloc), alloc);
  v.AddMember("bind", toJson(ingress.bind_, alloc), alloc);
  v.AddMember("port", ingress.port_, alloc);
  if (ingress.method_.has_value()) v.AddMember("method", toJson(*ingress.method_, alloc), alloc);
  if (ingress.password_.has_value())
    v.AddMember("password", toJson(*ingress.password_, alloc), alloc);
  if (ingress.tls_.has_value()) v.AddMember("tls", *ingress.tls_, alloc);
  if (ingress.certFile_.has_value())
    v.AddMember("cert_file", toJson(*ingress.certFile_, alloc), alloc);
  if (ingress.keyFile_.has_value())
    v.AddMember("key_file", toJson(*ingress.keyFile_, alloc), alloc);
  if (!ingress.destinations_.empty())
    v.AddMember("destinations",
                toJson(begin(ingress.destinations_), end(ingress.destinations_), alloc), alloc);
  if (ingress.balance_.has_value()) v.AddMember("balance", toJson(*ingress.balance_, alloc), alloc);

  return toString(v);
}

static string toString(vo::Egress const& evo)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember("type", toJson(evo.type_, alloc), alloc);
  if (evo.host_) v.AddMember("host", toJson(*evo.host_, alloc), alloc);
  if (evo.port_) v.AddMember("port", *evo.port_, alloc);
  if (evo.method_) v.AddMember("method", toJson(*evo.method_, alloc), alloc);
  if (evo.password_) v.AddMember("password", toJson(*evo.password_, alloc), alloc);
  if (evo.mode_) v.AddMember("mode", toJson(*evo.mode_, alloc), alloc);
  if (evo.delay_) v.AddMember("delay", *evo.delay_, alloc);
  if (evo.tls_) v.AddMember("tls", *evo.tls_, alloc);
  if (evo.insecure_) v.AddMember("insecure", *evo.insecure_, alloc);
  if (evo.caFile_) v.AddMember("ca_file", toJson(*evo.caFile_, alloc), alloc);

  return toString(v);
}

static string toString(vo::Rule const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (!rvo.range_.empty())
    v.AddMember("range", toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    v.AddMember("ingress_name", toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    v.AddMember("ingress_type", toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    v.AddMember("pattern", toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    v.AddMember("domain", toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    v.AddMember("country", toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);

  return toString(v);
}

static string toString(vo::Route const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (rvo.default_) v.AddMember("default", toJson(*rvo.default_, alloc), alloc);
  auto rules = Value{};
  rules.SetArray();
  for_each(cbegin(rvo.rules_), cend(rvo.rules_), [&rules](auto&& rule) {
    auto vo = Value{};
    vo.SetArray();
    for_each(cbegin(rule.first), cend(rule.first),
             [&vo](auto&& item) { vo.PushBack(toJson(item, alloc), alloc); });
    vo.PushBack(toJson(rule.second, alloc), alloc);
    rules.PushBack(move(vo), alloc);
  });
  v.AddMember("rules", rules, alloc);

  return toString(v);
}

static bool operator==(vo::Ingress const& lhs, vo::Ingress const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.bind_ == rhs.bind_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_ && lhs.tls_ == rhs.tls_ &&
         lhs.certFile_ == rhs.certFile_ && lhs.keyFile_ == rhs.keyFile_ &&
         equal(cbegin(lhs.destinations_), cend(lhs.destinations_), cbegin(rhs.destinations_),
               cend(rhs.destinations_), [](auto&& l, auto&& r) { return l == r; }) &&
         lhs.balance_ == rhs.balance_;
}

static bool operator==(vo::Egress const& lhs, vo::Egress const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_ && lhs.mode_ == rhs.mode_ &&
         lhs.delay_ == rhs.delay_ && lhs.tls_ == rhs.tls_ && lhs.insecure_ == rhs.insecure_ &&
         lhs.caFile_ == rhs.caFile_;
}

static bool operator==(vo::Rule const& lhs, vo::Rule const& rhs)
{
  return equal(begin(lhs.range_), end(lhs.range_), begin(rhs.range_), end(rhs.range_)) &&
         equal(begin(lhs.ingress_), end(lhs.ingress_), begin(rhs.ingress_), end(rhs.ingress_)) &&
         equal(begin(lhs.type_), end(lhs.type_), begin(rhs.type_), end(rhs.type_)) &&
         equal(begin(lhs.pattern_), end(lhs.pattern_), begin(rhs.pattern_), end(rhs.pattern_)) &&
         equal(begin(lhs.domain_), end(lhs.domain_), begin(rhs.domain_), end(rhs.domain_)) &&
         equal(begin(lhs.country_), end(lhs.country_), begin(rhs.country_), end(rhs.country_));
}

static bool operator==(vo::Route const& lhs, vo::Route const& rhs)
{
  return lhs.default_ == rhs.default_ &&
         equal(begin(lhs.rules_), end(lhs.rules_), begin(rhs.rules_), end(rhs.rules_));
}

BOOST_AUTO_TEST_SUITE(REST_PARSE)

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Type)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"invalid\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"direct\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"reject\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Default_Ones)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5, AdapterType::SS, AdapterType::TUNNEL}) {
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(defaultIngressJson(type))));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noType = defaultIngressJson(type);
    noType.RemoveMember("type");
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noType)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noBind = defaultEgressJson(type);
    noBind.RemoveMember("bind");
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBind)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = defaultEgressJson(type);
    noPort.RemoveMember("port");
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto vo = defaultIngressVO(type);
    BOOST_CHECK(vo == parse<vo::Ingress>(toString(vo)));

    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(vo)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Default_TLS_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultIngressJson(type);
    json.RemoveMember("tls");
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(json)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto origin = defaultIngressVO(type);
    parse<vo::Ingress>(toString(origin));

    auto emptyBind = origin;
    emptyBind.bind_.clear();
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyBind)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = origin;
    zeroPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_TLS_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noCertFile = defaultIngressJson(type);
    noCertFile["tls"] = true;
    noCertFile.AddMember("key_file", toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noCertFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyCertFile = defaultIngressJson(type);
    emptyCertFile["tls"] = true;
    emptyCertFile.AddMember("cert_file", toJson("", alloc), alloc);
    emptyCertFile.AddMember("key_file", toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyCertFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noKeyFile = defaultIngressJson(type);
    noKeyFile["tls"] = true;
    noKeyFile.AddMember("cert_file", toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noKeyFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyKeyFile = defaultIngressJson(type);
    emptyKeyFile["tls"] = true;
    emptyKeyFile.AddMember("cert_file", toJson(ph, alloc), alloc);
    emptyKeyFile.AddMember("key_file", toJson("", alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyKeyFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_TLS_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto vo = defaultIngressVO(type);
    vo.tls_ = true;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;

    auto json = defaultIngressJson(type);
    json["tls"] = true;
    json.AddMember("key_file", toJson(ph, alloc), alloc);
    json.AddMember("cert_file", toJson(ph, alloc), alloc);
    json.AddMember("password", toJson(ph, alloc), alloc);
    json.AddMember("method", toJson(ph, alloc), alloc);

    BOOST_CHECK(vo == parse<vo::Ingress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Incorrect_Type_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto credentials = Value{};
    credentials.SetArray();
    auto json = defaultIngressJson(type);
    json.AddMember("credentials", credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Empty_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto emptyCred = Value{};
    emptyCred.SetObject();
    auto json = defaultIngressJson(type);
    json.AddMember("credentials", emptyCred, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Too_Long_Name)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto tooLongName = toJson(string(256_sz, 'n'), alloc);
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(tooLongName, ph, alloc);
    auto json = defaultIngressJson(type);
    json.AddMember("credentials", credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Too_Long_Password)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto tooLongPassword = toJson(string(256_sz, 'p'), alloc);
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(ph, tooLongPassword, alloc);
    auto json = defaultIngressJson(type);
    json.AddMember("credentials", credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(ph, ph, alloc);

    auto json = defaultIngressJson(type);
    json.AddMember("credentials", credentials, alloc);

    auto ivo = parse<vo::Ingress>(json);
    BOOST_CHECK_EQUAL(1_sz, ivo.credentials_.size());
    auto it = ivo.credentials_.find(ph);
    BOOST_CHECK(it != cend(ivo.credentials_));
    BOOST_CHECK_EQUAL(ph, it->first);
    BOOST_CHECK_EQUAL(ph, it->second);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Additional_Fields)
{
  auto json = defaultIngressJson(AdapterType::SS);
  json.AddMember("tls", false, alloc);
  json.AddMember("cert_file", toJson(ph, alloc), alloc);
  json.AddMember("key_file", toJson(ph, alloc), alloc);
  BOOST_CHECK(defaultIngressVO(AdapterType::SS) == parse<vo::Ingress>(json));
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Empty_Fields)
{
  auto origin = defaultIngressVO(AdapterType::SS);
  auto holder = parse<vo::Ingress>(toString(origin));

  auto emptyBind = origin;
  emptyBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyBind)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Mandatory_Fields)
{
  auto noType = defaultIngressJson(AdapterType::TUNNEL);
  noType.RemoveMember("type");
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noType)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noBind = defaultIngressJson(AdapterType::TUNNEL);
  noBind.RemoveMember("bind");
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBind)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPort = defaultIngressJson(AdapterType::TUNNEL);
  noPort.RemoveMember("port");
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noDest = defaultIngressJson(AdapterType::TUNNEL);
  noDest.RemoveMember("destinations");
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noDest)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noBalance = defaultIngressJson(AdapterType::TUNNEL);
  noBalance.RemoveMember("balance");
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBalance)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Empty_Destinations)
{
  auto json = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(json.HasMember("destinations"));
  json["destinations"].RemoveAllMembers();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(json)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Empty_Host)
{
  auto json = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(json.HasMember("destinations"));
  json["destinations"].AddMember("", 1, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(json)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Non_Integer_Value)
{
  auto host = "localhost";

  auto bValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(bValue.HasMember("destinations"));
  bValue["destinations"][host] = true;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(bValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto sValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(sValue.HasMember("destinations"));
  sValue["destinations"][host] = ph;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(sValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto dValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(dValue.HasMember("destinations"));
  dValue["destinations"][host] = 0.0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(dValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto oValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(oValue.HasMember("destinations"));
  oValue["destinations"][host] = Value{}.SetObject();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(oValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto aValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(aValue.HasMember("destinations"));
  aValue["destinations"][host] = Value{}.SetArray();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(aValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Invalid_Port)
{
  auto negative = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(negative.HasMember("destinations"));
  negative["destinations"].AddMember(ph, -1, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(negative)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto huge = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(huge.HasMember("destinations"));
  huge["destinations"].AddMember(ph, 65536, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(huge)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Invalid_Balance_Type)
{
  auto b = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(b.HasMember("balance"));
  b["balance"] = true;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(b)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto i = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(b.HasMember("balance"));
  i["balance"] = 0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(i)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto d = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(d.HasMember("balance"));
  d["balance"] = 0.0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(d)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto o = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(o.HasMember("balance"));
  o["balance"] = Value{}.SetObject();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(o)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto a = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(a.HasMember("balance"));
  a["balance"] = Value{}.SetArray();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(a)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Invalid_Balance_String)
{
  auto empty = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(empty.HasMember("balance"));
  empty["balance"] = "";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(empty)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto invalid = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(invalid.HasMember("balance"));
  invalid["balance"] = "invalid balance";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(invalid)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Balance)
{
  for (auto p : {make_pair("random", BalanceType::RANDOM),
                 make_pair("round_robin", BalanceType::ROUND_ROBIN),
                 make_pair("least_conn", BalanceType::LEAST_CONN)}) {
    auto&& [s, v] = p;

    auto json = defaultIngressJson(AdapterType::TUNNEL);
    BOOST_CHECK(json.HasMember("balance"));
    json["balance"].SetString(s, alloc);

    auto vo = defaultIngressVO(AdapterType::TUNNEL);
    vo.balance_ = v;

    BOOST_CHECK(vo == parse<vo::Ingress>(toString(json)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(huge), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Default_Ones)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::HTTP, AdapterType::REJECT,
                    AdapterType::SOCKS5, AdapterType::SS}) {
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(toString(defaultEgressJson(type))));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_Direct_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::DIRECT);
  vo.delay_ = 0_u16;
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.mode_ = DelayMode::FIXED;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(defaultEgressVO(AdapterType::DIRECT) == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Default_Mode)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.delay_.reset();
  vo.mode_.reset();
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(defaultEgressVO(AdapterType::REJECT) == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Invalid_Mode)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"invalid mode\"}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Random_Additional_Fields)
{
  auto origin = defaultEgressVO(AdapterType::REJECT);
  origin.mode_ = DelayMode::RANDOM;
  origin.delay_.reset();
  BOOST_CHECK(origin == parse<vo::Egress>(toString(origin)));

  auto vo = origin;
  vo.delay_ = 0_u16;
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(origin == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Missing_Delay)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(
      parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":\"1\"}"), Exception,
      verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Delay_Out_Of_Range)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":-1}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":301}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Fixed)
{
  for (auto i = 0_u16; i <= 300_u16; ++i) {
    auto vo = defaultEgressVO(AdapterType::REJECT);
    vo.delay_ = i;
    BOOST_CHECK(vo == parse<vo::Egress>(toString(vo)));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noHost = defaultEgressJson(type);
    noHost.RemoveMember("host");
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyHost = defaultEgressJson(type);
    emptyHost["host"] = "";
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = defaultEgressJson(type);
    noPort.RemoveMember("port");
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = defaultEgressJson(type);
    zeroPort["port"] = 0;
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.AddMember("method", toJson(ph, alloc), alloc);
    json.AddMember("password", toJson(ph, alloc), alloc);
    json.AddMember("mode", toJson(ph, alloc), alloc);
    json.AddMember("delay", 0, alloc);
    json.AddMember("insecure", true, alloc);
    json.AddMember("ca_file", toJson(ph, alloc), alloc);
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_TLS_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.RemoveMember("tls");
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_Insecure_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json["tls"] = true;
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Insecure_With_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json["tls"] = true;
    json.AddMember("insecure", true, alloc);
    json.AddMember("ca_file", toJson(ph, alloc), alloc);
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = true;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Secure_With_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json["tls"] = true;
    json.AddMember("ca_file", toJson(ph, alloc), alloc);
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    vo.caFile_ = ph;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Secure_With_Empty_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json["tls"] = true;
    json.AddMember("ca_file", toJson("", alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Incorrect_Type_Credential)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto obj = Value{};
    obj.SetObject();
    auto json = defaultEgressJson(type);
    json.AddMember("credential", obj, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Incorrect_Credential_Pair)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    for (auto count = 0; count < 10; ++count) {
      if (count == 2) continue;
      auto ary = Value{};
      ary.SetArray();
      for (auto i = 0; i < count; ++i) ary.PushBack(ph, alloc);
      auto json = defaultEgressJson(type);
      json.AddMember("credential", ary, alloc);
      BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                            verifyException<PichiError::BAD_JSON>);
    }
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Too_Long_Name)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto ary = Value{};
    ary.SetArray();
    ary.PushBack(toJson(string(256_sz, 'n'), alloc), alloc);
    ary.PushBack(ph, alloc);
    auto json = defaultEgressJson(type);
    json.AddMember("credential", ary, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Too_Long_Password)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto ary = Value{};
    ary.SetArray();
    ary.PushBack(ph, alloc);
    ary.PushBack(toJson(string(256_sz, 'p'), alloc), alloc);
    auto json = defaultEgressJson(type);
    json.AddMember("credential", ary, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS_Mandatory_Fields)
{
  auto origin = defaultEgressVO(AdapterType::SS);
  auto holder = parse<vo::Egress>(toString(origin));

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPort = origin;
  noPort.port_ = 0_u16;
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS_Additional_Fields)
{
  auto json = defaultEgressJson(AdapterType::SS);
  json.AddMember("mode", toJson(ph, alloc), alloc);
  json.AddMember("delay", 0, alloc);
  json.AddMember("tls", true, alloc);
  json.AddMember("insecure", true, alloc);
  json.AddMember("ca_file", toJson(ph, alloc), alloc);
  BOOST_CHECK(defaultEgressVO(AdapterType::SS) == parse<vo::Egress>(json));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(huge), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Rule_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Rule)
{
  auto const expect = vo::Rule{};
  auto fact = parse<vo::Rule>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Fields)
{
  auto const origin = vo::Rule{};
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Document{};
    auto array = Value{};
    expect.SetObject();
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return toString(expect);
  };

  auto range = origin;
  range.range_.emplace_back(ph);
  auto fact = parse<vo::Rule>(generate("range", ph));
  BOOST_CHECK(range == fact);

  auto ingress = origin;
  ingress.ingress_.emplace_back(ph);
  fact = parse<vo::Rule>(generate("ingress_name", ph));
  BOOST_CHECK(ingress == fact);

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  fact = parse<vo::Rule>(generate("ingress_type", AdapterType::DIRECT));
  BOOST_CHECK(type == fact);

  auto pattern = origin;
  pattern.pattern_.emplace_back(ph);
  fact = parse<vo::Rule>(generate("pattern", ph));
  BOOST_CHECK(pattern == fact);

  auto domain = origin;
  domain.domain_.emplace_back(ph);
  fact = parse<vo::Rule>(generate("domain", ph));
  BOOST_CHECK(domain == fact);

  auto country = origin;
  country.country_.emplace_back(ph);
  fact = parse<vo::Rule>(generate("country", ph));
  BOOST_CHECK(country == fact);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Empty_Fields_Content)
{
  auto const origin = vo::Rule{};
  parse<vo::Rule>(toString(origin));

  auto range = origin;
  range.range_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>(toString(range)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto ingress = origin;
  ingress.ingress_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>(toString(ingress)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto pattern = origin;
  pattern.pattern_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>(toString(pattern)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto domain = origin;
  domain.domain_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>(toString(domain)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto country = origin;
  country.country_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<vo::Rule>(toString(country)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Superfluous_Field)
{
  BOOST_CHECK(vo::Rule{} == parse<vo::Rule>("{\"superfluous_field\":\"none\"}"));
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Empty_Fields)
{
  auto const emptyDefault = vo::Route{""};
  BOOST_CHECK_EXCEPTION(parse<vo::Route>(toString(emptyDefault)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Rules_Type)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": 0}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": 0.0}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": \"not an array\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": {}}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Rules_Items_Type)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [0]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [0.0]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [\"not an array\"]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [{}]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Rules_Not_Pair)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [[]]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Route>("{\"rules\": [[\"a\"]]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route)
{
  auto expect = vo::Route{};

  auto fact = parse<vo::Route>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.emplace(ph);
  fact = parse<vo::Route>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.reset();
  expect.rules_.emplace_back(make_pair(vector<string>{ph}, ph));
  fact = parse<vo::Route>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.emplace(ph);
  fact = parse<vo::Route>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.rules_.clear();
  fact = parse<vo::Route>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_SUITE_END()
