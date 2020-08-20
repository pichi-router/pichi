#define BOOST_TEST_MODULE pichi rest_parse test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/common/exception.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/keys.hpp>
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
using vo::toString;

using pichi::AdapterType;

static string toString(vo::Egress const& evo)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember(vo::egress::TYPE, toJson(evo.type_, alloc), alloc);
  if (evo.host_) v.AddMember(vo::egress::HOST, toJson(*evo.host_, alloc), alloc);
  if (evo.port_) v.AddMember(vo::egress::PORT, *evo.port_, alloc);
  if (evo.method_) v.AddMember(vo::egress::METHOD, toJson(*evo.method_, alloc), alloc);
  if (evo.password_) v.AddMember(vo::egress::PASSWORD, toJson(*evo.password_, alloc), alloc);
  if (evo.mode_) v.AddMember(vo::egress::MODE, toJson(*evo.mode_, alloc), alloc);
  if (evo.delay_) v.AddMember(vo::egress::DELAY, *evo.delay_, alloc);
  if (evo.tls_) v.AddMember(vo::egress::TLS, *evo.tls_, alloc);
  if (evo.insecure_) v.AddMember(vo::egress::INSECURE, *evo.insecure_, alloc);
  if (evo.caFile_) v.AddMember(vo::egress::CA_FILE, toJson(*evo.caFile_, alloc), alloc);

  return toString(v);
}

static string toString(vo::Rule const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (!rvo.range_.empty())
    v.AddMember(vo::rule::RANGE, toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    v.AddMember(vo::rule::INGRESS, toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    v.AddMember(vo::rule::TYPE, toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    v.AddMember(vo::rule::PATTERN, toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    v.AddMember(vo::rule::DOMAIN_NAME, toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    v.AddMember(vo::rule::COUNTRY, toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);

  return toString(v);
}

static string toString(vo::Route const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (rvo.default_) v.AddMember(vo::route::DEFAULT, toJson(*rvo.default_, alloc), alloc);
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
  v.AddMember(vo::route::RULES, rules, alloc);

  return toString(v);
}

BOOST_AUTO_TEST_SUITE(REST_PARSE)

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
    noHost.RemoveMember(vo::egress::HOST);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyHost = defaultEgressJson(type);
    emptyHost[vo::egress::HOST] = "";
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = defaultEgressJson(type);
    noPort.RemoveMember(vo::egress::PORT);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = defaultEgressJson(type);
    zeroPort[vo::egress::PORT] = 0;
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.AddMember(vo::egress::METHOD, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::PASSWORD, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::MODE, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::DELAY, 0, alloc);
    json.AddMember(vo::egress::INSECURE, true, alloc);
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_TLS_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.RemoveMember(vo::egress::TLS);
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_Insecure_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
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
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::INSECURE, true, alloc);
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
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
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
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
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::CA_FILE, toJson("", alloc), alloc);
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
    json.AddMember(vo::egress::CREDENTIAL, obj, alloc);
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
      json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
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
    json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
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
    json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
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
  json.AddMember(vo::egress::MODE, toJson(ph, alloc), alloc);
  json.AddMember(vo::egress::DELAY, 0, alloc);
  json.AddMember(vo::egress::TLS, true, alloc);
  json.AddMember(vo::egress::INSECURE, true, alloc);
  json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
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
  auto fact = parse<vo::Rule>(generate(vo::rule::RANGE, ph));
  BOOST_CHECK(range == fact);

  auto ingress = origin;
  ingress.ingress_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::INGRESS, ph));
  BOOST_CHECK(ingress == fact);

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  fact = parse<vo::Rule>(generate(vo::rule::TYPE, AdapterType::DIRECT));
  BOOST_CHECK(type == fact);

  auto pattern = origin;
  pattern.pattern_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::PATTERN, ph));
  BOOST_CHECK(pattern == fact);

  auto domain = origin;
  domain.domain_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::DOMAIN_NAME, ph));
  BOOST_CHECK(domain == fact);

  auto country = origin;
  country.country_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::COUNTRY, ph));
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
