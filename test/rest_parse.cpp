#define BOOST_TEST_MODULE pichi rest_parse test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/exception.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string_view>

using namespace std;
using namespace rapidjson;
using namespace pichi;
using namespace pichi::api;

static decltype(auto) ph = "placeholder";

static string toString(Value const& v)
{
  auto buf = StringBuffer{};
  auto writer = Writer<StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

static string toString(IngressVO const& ingress)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("type", toJson(ingress.type_, alloc), alloc);
  doc.AddMember("bind", toJson(ingress.bind_, alloc), alloc);
  doc.AddMember("port", ingress.port_, alloc);
  if (ingress.method_.has_value()) doc.AddMember("method", toJson(*ingress.method_, alloc), alloc);
  if (ingress.password_.has_value())
    doc.AddMember("password", toJson(*ingress.password_, alloc), alloc);

  return toString(doc);
}

static string toString(EgressVO const& evo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("type", toJson(evo.type_, alloc), alloc);
  if (evo.host_) doc.AddMember("host", toJson(*evo.host_, alloc), alloc);
  if (evo.port_) doc.AddMember("port", *evo.port_, alloc);
  if (evo.method_) doc.AddMember("method", toJson(*evo.method_, alloc), alloc);
  if (evo.password_) doc.AddMember("password", toJson(*evo.password_, alloc), alloc);

  return toString(doc);
}

static string toString(RuleVO const& rvo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  if (!rvo.range_.empty())
    doc.AddMember("range", toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    doc.AddMember("ingress_name", toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    doc.AddMember("ingress_type", toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    doc.AddMember("pattern", toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    doc.AddMember("domain", toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    doc.AddMember("country", toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);

  return toString(doc);
}

static string toString(RouteVO const& rvo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  if (rvo.default_) doc.AddMember("default", toJson(*rvo.default_, alloc), alloc);
  auto rules = Value{};
  rules.SetArray();
  for_each(cbegin(rvo.rules_), cend(rvo.rules_), [&rules, &alloc](auto&& rule) {
    auto vo = Value{};
    vo.SetArray();
    vo.PushBack(toJson(rule.first, alloc), alloc);
    vo.PushBack(toJson(rule.second, alloc), alloc);
    rules.PushBack(move(vo), alloc);
  });
  doc.AddMember("rules", rules, alloc);

  return toString(doc);
}

static bool operator==(IngressVO const& lhs, IngressVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.bind_ == rhs.bind_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_;
}

static bool operator==(EgressVO const& lhs, EgressVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_;
}

static bool operator==(RuleVO const& lhs, RuleVO const& rhs)
{
  return equal(begin(lhs.range_), end(lhs.range_), begin(rhs.range_), end(rhs.range_)) &&
         equal(begin(lhs.ingress_), end(lhs.ingress_), begin(rhs.ingress_), end(rhs.ingress_)) &&
         equal(begin(lhs.type_), end(lhs.type_), begin(rhs.type_), end(rhs.type_)) &&
         equal(begin(lhs.pattern_), end(lhs.pattern_), begin(rhs.pattern_), end(rhs.pattern_)) &&
         equal(begin(lhs.domain_), end(lhs.domain_), begin(rhs.domain_), end(rhs.domain_)) &&
         equal(begin(lhs.country_), end(lhs.country_), begin(rhs.country_), end(rhs.country_));
}

static bool operator==(RouteVO const& lhs, RouteVO const& rhs)
{
  return lhs.default_ == rhs.default_ &&
         equal(begin(lhs.rules_), end(lhs.rules_), begin(rhs.rules_), end(rhs.rules_));
}

static auto nonUpdating = [](auto&&, auto&&) { BOOST_ERROR("unexpected invocation"); };

BOOST_AUTO_TEST_SUITE(REST_PARSE)

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Direct_Reject_Additional_Fields)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto const expect = IngressVO{type};
    auto fact = parse<IngressVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<IngressVO>(toString(IngressVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_Socks5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = IngressVO{type, ph, 1};
    auto fact = parse<IngressVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<IngressVO>(toString(IngressVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_Socks5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const origin = IngressVO{type, ph, 1};
    parse<IngressVO>(toString(origin));

    auto emptyBind = origin;
    emptyBind.bind_.clear();
    BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(emptyBind)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = origin;
    zeroPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Empty_Fields)
{
  auto const origin = IngressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto holder = parse<IngressVO>(toString(origin));

  auto emptyBind = origin;
  emptyBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(emptyBind)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(huge), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Base)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = IngressVO{type, ph, 1};
    auto fact = parse<IngressVO>(toString(expect));
    BOOST_CHECK(fact == expect);

    auto withMethod = expect;
    withMethod.method_ = CryptoMethod::AES_128_CFB;
    fact = parse<IngressVO>(toString(withMethod));
    BOOST_CHECK(fact == expect);

    auto withPassword = expect;
    withPassword.password_ = ph;
    fact = parse<IngressVO>(toString(withPassword));
    BOOST_CHECK(fact == expect);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS)
{
  auto const expect = IngressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = parse<IngressVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<EgressVO>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<EgressVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Direct_Reject_Additional_Fields)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto const expect = EgressVO{type};
    auto fact = parse<EgressVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<EgressVO>(toString(EgressVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const origin = EgressVO{type, ph, 1};
    auto holder = parse<EgressVO>(toString(origin));

    auto noHost = origin;
    noHost.host_.reset();
    BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyHost = origin;
    emptyHost.host_->clear();
    BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(emptyHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = origin;
    zeroPort.port_.reset();
    BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = origin;
    noPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS_Empty_Fields)
{
  auto const origin = EgressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto holder = parse<EgressVO>(toString(origin));

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(emptyHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_.reset();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPort = origin;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<EgressVO>(huge), Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = EgressVO{type, ph, 1};
    auto fact = parse<EgressVO>(toString(expect));
    BOOST_CHECK(fact == expect);

    auto withMethod = expect;
    withMethod.method_ = CryptoMethod::AES_128_CFB;
    fact = parse<EgressVO>(toString(withMethod));
    BOOST_CHECK(fact == expect);

    auto withPassword = expect;
    withPassword.password_ = ph;
    fact = parse<EgressVO>(toString(withPassword));
    BOOST_CHECK(fact == expect);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS)
{
  auto const expect = EgressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = parse<EgressVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Rule)
{
  auto const expect = RuleVO{};
  auto fact = parse<RuleVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Fields)
{
  auto const origin = RuleVO{};
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Document{};
    auto& alloc = expect.GetAllocator();
    auto array = Value{};
    expect.SetObject();
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return toString(expect);
  };

  auto range = origin;
  range.range_.emplace_back(ph);
  auto fact = parse<RuleVO>(generate("range", ph));
  BOOST_CHECK(range == fact);

  auto ingress = origin;
  ingress.ingress_.emplace_back(ph);
  fact = parse<RuleVO>(generate("ingress_name", ph));
  BOOST_CHECK(ingress == fact);

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  fact = parse<RuleVO>(generate("ingress_type", AdapterType::DIRECT));
  BOOST_CHECK(type == fact);

  auto pattern = origin;
  pattern.pattern_.emplace_back(ph);
  fact = parse<RuleVO>(generate("pattern", ph));
  BOOST_CHECK(pattern == fact);

  auto domain = origin;
  domain.domain_.emplace_back(ph);
  fact = parse<RuleVO>(generate("domain", ph));
  BOOST_CHECK(domain == fact);

  auto country = origin;
  country.country_.emplace_back(ph);
  fact = parse<RuleVO>(generate("country", ph));
  BOOST_CHECK(country == fact);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Empty_Fields_Content)
{
  auto const origin = RuleVO{};
  parse<RuleVO>(toString(origin));

  auto range = origin;
  range.range_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(range)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto ingress = origin;
  ingress.ingress_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(ingress)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto pattern = origin;
  pattern.pattern_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(pattern)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto domain = origin;
  domain.domain_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(domain)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto country = origin;
  country.country_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(country)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Superfluous_Field)
{
  BOOST_CHECK(RuleVO{} == parse<RuleVO>("{\"superfluous_field\":\"none\"}"));
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Empty_Fields)
{
  auto const emptyDefault = RouteVO{""};
  BOOST_CHECK_EXCEPTION(parse<RouteVO>(toString(emptyDefault)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Rules_Type)
{
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": 0}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": 0.0}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": \"not an array\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": {}}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Rules_Items_Type)
{
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [0]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [0.0]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [\"not an array\"]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [{}]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Rules_Not_Pair)
{
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [[]]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [[\"a\"]]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("{\"rules\": [[\"a\",\"b\",\"c\"]]}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Route)
{
  auto expect = RouteVO{};

  auto fact = parse<RouteVO>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.emplace(ph);
  fact = parse<RouteVO>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.reset();
  expect.rules_.emplace_back(make_pair(ph, ph));
  fact = parse<RouteVO>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.default_.emplace(ph);
  fact = parse<RouteVO>(toString(expect));
  BOOST_CHECK(fact == expect);

  expect.rules_.clear();
  fact = parse<RouteVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_SUITE_END()
