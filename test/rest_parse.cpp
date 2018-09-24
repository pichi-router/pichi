#define BOOST_TEST_MODULE pichi rest_parse test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/exception.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <string_view>
#include <unordered_map>

using namespace std;
using namespace rapidjson;
using namespace pichi;
using namespace pichi::api;

static decltype(auto) ph = "placeholder";
static auto stub = [](auto&&, auto&&) {};

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

static string toString(OutboundVO const& ovo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("type", toJson(ovo.type_, alloc), alloc);
  if (ovo.host_) doc.AddMember("host", toJson(*ovo.host_, alloc), alloc);
  if (ovo.port_) doc.AddMember("port", *ovo.port_, alloc);
  if (ovo.method_) doc.AddMember("method", toJson(*ovo.method_, alloc), alloc);
  if (ovo.password_) doc.AddMember("password", toJson(*ovo.password_, alloc), alloc);

  return toString(doc);
}

static string toString(RuleVO const& rvo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("outbound", toJson(rvo.outbound_, alloc), alloc);
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
  doc.AddMember("rules", toJson(begin(rvo.rules_), end(rvo.rules_), alloc), alloc);

  return toString(doc);
}

static bool operator==(IngressVO const& lhs, IngressVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.bind_ == rhs.bind_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_;
}

static bool operator==(OutboundVO const& lhs, OutboundVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_;
}

static bool operator==(RuleVO const& lhs, RuleVO const& rhs)
{
  return lhs.outbound_ == rhs.outbound_ &&
         equal(begin(lhs.range_), end(lhs.range_), begin(rhs.range_), end(rhs.range_)) &&
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

BOOST_AUTO_TEST_SUITE(REST_PARSE)

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("not a json"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("not a json", stub), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<IngressVO>("[\"not a json object\"]", stub), Exception,
                        verifyException<PichiError::MISC>);
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
                          verifyException<PichiError::MISC>);

    auto zeroPort = origin;
    zeroPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(zeroPort)), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Empty_Fields)
{
  auto const origin = IngressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto holder = parse<IngressVO>(toString(origin));

  auto emptyBind = origin;
  emptyBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(emptyBind)), Exception,
                        verifyException<PichiError::MISC>);

  auto zeroPort = origin;
  zeroPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(zeroPort)), Exception,
                        verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(noMethod)), Exception,
                        verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(noPassword)), Exception,
                        verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(negative), Exception, verifyException<PichiError::MISC>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<IngressVO>(huge), Exception, verifyException<PichiError::MISC>);
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

BOOST_AUTO_TEST_CASE(parse_IngressVO_Empty_Pack)
{
  auto m = unordered_map<string, IngressVO>{};
  parse<IngressVO>("{}", [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.empty());
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Pack)
{
  auto m = unordered_map<string, IngressVO>{};
  parse<IngressVO>("{\"placeholder\":{\"type\":\"direct\"}}",
                   [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.size() == 1);
  auto it = m.find(ph);
  BOOST_CHECK(it != end(m));
  BOOST_CHECK(it->second == IngressVO{AdapterType::DIRECT});
}

BOOST_AUTO_TEST_CASE(parse_Outbound_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>("not a json"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>("not a json", stub), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>("[\"not a json object\"]", stub), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Outbound_Direct_Reject_Additional_Fields)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto const expect = OutboundVO{type};
    auto fact = parse<OutboundVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<OutboundVO>(toString(OutboundVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_Outbound_HTTP_SOCKS5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const origin = OutboundVO{type, ph, 1};
    auto holder = parse<OutboundVO>(toString(origin));

    auto noHost = origin;
    noHost.host_.reset();
    BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noHost)), Exception,
                          verifyException<PichiError::MISC>);

    auto emptyHost = origin;
    emptyHost.host_->clear();
    BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(emptyHost)), Exception,
                          verifyException<PichiError::MISC>);

    auto zeroPort = origin;
    zeroPort.port_.reset();
    BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(zeroPort)), Exception,
                          verifyException<PichiError::MISC>);

    auto noPort = origin;
    noPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noPort)), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Outbound_SS_Empty_Fields)
{
  auto const origin = OutboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto holder = parse<OutboundVO>(toString(origin));

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noHost)), Exception,
                        verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(emptyHost)), Exception,
                        verifyException<PichiError::MISC>);

  auto zeroPort = origin;
  zeroPort.port_.reset();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(zeroPort)), Exception,
                        verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noPort)), Exception,
                        verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noMethod)), Exception,
                        verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(noPassword)), Exception,
                        verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Outbound_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(negative), Exception, verifyException<PichiError::MISC>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<OutboundVO>(huge), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Outbound_HTTP_SOCKS5)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = OutboundVO{type, ph, 1};
    auto fact = parse<OutboundVO>(toString(expect));
    BOOST_CHECK(fact == expect);

    auto withMethod = expect;
    withMethod.method_ = CryptoMethod::AES_128_CFB;
    fact = parse<OutboundVO>(toString(withMethod));
    BOOST_CHECK(fact == expect);

    auto withPassword = expect;
    withPassword.password_ = ph;
    fact = parse<OutboundVO>(toString(withPassword));
    BOOST_CHECK(fact == expect);
  }
}

BOOST_AUTO_TEST_CASE(parse_Outbound_SS)
{
  auto const expect = OutboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = parse<OutboundVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Outbound_Empty_Pack)
{
  auto m = unordered_map<string, OutboundVO>{};
  parse<OutboundVO>("{}", [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.empty());
}

BOOST_AUTO_TEST_CASE(parse_Outbound_Pack)
{
  auto m = unordered_map<string, OutboundVO>{};
  parse<OutboundVO>("{\"placeholder\":{\"type\":\"direct\"}}",
                    [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.size() == 1);
  auto it = m.find(ph);
  BOOST_CHECK(it != end(m));
  BOOST_CHECK(it->second == OutboundVO{AdapterType::DIRECT});
}

BOOST_AUTO_TEST_CASE(parse_Rule_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("not a json"), Exception, verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("not a json", stub), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<RuleVO>("[\"not a json object\"]", stub), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Rule_Empty_Fileds)
{
  auto const origin = RuleVO{ph};
  auto holder = parse<RuleVO>(toString(origin));

  auto emptyOutbound = origin;
  emptyOutbound.outbound_.clear();
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(emptyOutbound)), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Rule)
{
  auto const expect = RuleVO{ph};
  auto fact = parse<RuleVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Fields)
{
  auto const origin = RuleVO{ph};
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Document{};
    auto& alloc = expect.GetAllocator();
    auto array = Value{};
    expect.SetObject();
    expect.AddMember("outbound", ph, alloc);
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
  auto const origin = RuleVO{ph};
  parse<RuleVO>(toString(origin));

  auto range = origin;
  range.range_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(range)), Exception,
                        verifyException<PichiError::MISC>);

  auto ingress = origin;
  ingress.ingress_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(ingress)), Exception,
                        verifyException<PichiError::MISC>);

  auto pattern = origin;
  pattern.pattern_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(pattern)), Exception,
                        verifyException<PichiError::MISC>);

  auto domain = origin;
  domain.domain_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(domain)), Exception,
                        verifyException<PichiError::MISC>);

  auto country = origin;
  country.country_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(country)), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Superfluous_Field)
{
  auto const expect = RuleVO{ph};
  auto fact = parse<RuleVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_Empty_Pack)
{
  auto m = unordered_map<string, RuleVO>{};
  parse<RuleVO>("{}", [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.empty());
}

BOOST_AUTO_TEST_CASE(parse_Rule_Pack)
{
  auto m = unordered_map<string, RuleVO>{};
  parse<RuleVO>("{\"placeholder\":{\"outbound\":\"placeholder\"}}",
                [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.size() == 1);
  auto it = m.find(ph);
  BOOST_CHECK(it != end(m));
  BOOST_CHECK(it->second == RuleVO{ph});
}

BOOST_AUTO_TEST_CASE(parse_Route_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("not a json"), Exception, verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<RouteVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Route_Empty_Fields)
{
  auto const emptyDefault = RouteVO{""};
  BOOST_CHECK_EXCEPTION(parse<RouteVO>(toString(emptyDefault)), Exception,
                        verifyException<PichiError::MISC>);
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
  expect.rules_.emplace_back(ph);
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
