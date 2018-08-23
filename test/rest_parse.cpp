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

static string toString(InboundVO const& ivo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("type", toJson(ivo.type_, alloc), alloc);
  doc.AddMember("bind", toJson(ivo.bind_, alloc), alloc);
  doc.AddMember("port", ivo.port_, alloc);
  if (ivo.method_.has_value()) doc.AddMember("method", toJson(ivo.method_.value(), alloc), alloc);
  if (ivo.password_.has_value())
    doc.AddMember("password", toJson(ivo.password_.value(), alloc), alloc);

  return toString(doc);
}

static string toString(OutboundVO const& ovo)
{
  auto doc = Document{};
  auto& alloc = doc.GetAllocator();
  doc.SetObject();

  doc.AddMember("type", toJson(ovo.type_, alloc), alloc);
  if (ovo.host_) doc.AddMember("host", toJson(ovo.host_.value(), alloc), alloc);
  if (ovo.port_) doc.AddMember("port", ovo.port_.value(), alloc);
  if (ovo.method_) doc.AddMember("method", toJson(ovo.method_.value(), alloc), alloc);
  if (ovo.password_) doc.AddMember("password", toJson(ovo.password_.value(), alloc), alloc);

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
  if (!rvo.inbound_.empty())
    doc.AddMember("inbound_name", toJson(begin(rvo.inbound_), end(rvo.inbound_), alloc), alloc);
  if (!rvo.type_.empty())
    doc.AddMember("inbound_type", toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
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

  if (rvo.default_) doc.AddMember("default", toJson(rvo.default_.value(), alloc), alloc);
  doc.AddMember("rules", toJson(begin(rvo.rules_), end(rvo.rules_), alloc), alloc);

  return toString(doc);
}

static bool operator==(InboundVO const& lhs, InboundVO const& rhs)
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
         equal(begin(lhs.inbound_), end(lhs.inbound_), begin(rhs.inbound_), end(rhs.inbound_)) &&
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

BOOST_AUTO_TEST_CASE(parse_Inbound_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<InboundVO>("not a json"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<InboundVO>("not a json", stub), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<InboundVO>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::MISC>);
  BOOST_CHECK_EXCEPTION(parse<InboundVO>("[\"not a json object\"]", stub), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Inbound_Direct_Reject_Additional_Fields)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::REJECT}) {
    auto const expect = InboundVO{type};
    auto fact = parse<InboundVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<InboundVO>(toString(InboundVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_Inbound_HTTP_Socks5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = InboundVO{type, ph, 1};
    auto fact = parse<InboundVO>(toString(expect));
    BOOST_CHECK(expect == fact);

    fact = parse<InboundVO>(toString(InboundVO{type, ph, 1, CryptoMethod::AES_128_CFB, ph}));
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(parse_Inbound_HTTP_Socks5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const origin = InboundVO{type, ph, 1};
    parse<InboundVO>(toString(origin));

    auto emptyBind = origin;
    emptyBind.bind_.clear();
    BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(emptyBind)), Exception,
                          verifyException<PichiError::MISC>);

    auto zeroPort = origin;
    zeroPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(zeroPort)), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Inbound_SS_Empty_Fields)
{
  auto const origin = InboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto holder = parse<InboundVO>(toString(origin));

  auto emptyBind = origin;
  emptyBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(emptyBind)), Exception,
                        verifyException<PichiError::MISC>);

  auto zeroPort = origin;
  zeroPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(zeroPort)), Exception,
                        verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(noMethod)), Exception,
                        verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(noPassword)), Exception,
                        verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Inbound_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(negative), Exception, verifyException<PichiError::MISC>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<InboundVO>(huge), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(parse_Inbound_Base)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto const expect = InboundVO{type, ph, 1};
    auto fact = parse<InboundVO>(toString(expect));
    BOOST_CHECK(fact == expect);

    auto withMethod = expect;
    withMethod.method_ = CryptoMethod::AES_128_CFB;
    fact = parse<InboundVO>(toString(withMethod));
    BOOST_CHECK(fact == expect);

    auto withPassword = expect;
    withPassword.password_ = ph;
    fact = parse<InboundVO>(toString(withPassword));
    BOOST_CHECK(fact == expect);
  }
}

BOOST_AUTO_TEST_CASE(parse_Inbound_SS)
{
  auto const expect = InboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = parse<InboundVO>(toString(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Inbound_Empty_Pack)
{
  auto m = unordered_map<string, InboundVO>{};
  parse<InboundVO>("{}", [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.empty());
}

BOOST_AUTO_TEST_CASE(parse_Inbound_Pack)
{
  auto m = unordered_map<string, InboundVO>{};
  parse<InboundVO>("{\"placeholder\":{\"type\":\"direct\"}}",
                   [&](auto&& k, auto&& v) { m[k] = v; });
  BOOST_CHECK(m.size() == 1);
  auto it = m.find(ph);
  BOOST_CHECK(it != end(m));
  BOOST_CHECK(it->second == InboundVO{AdapterType::DIRECT});
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

  auto inbound = origin;
  inbound.inbound_.emplace_back(ph);
  fact = parse<RuleVO>(generate("inbound_name", ph));
  BOOST_CHECK(inbound == fact);

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  fact = parse<RuleVO>(generate("inbound_type", AdapterType::DIRECT));
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

  auto inbound = origin;
  inbound.inbound_.emplace_back("");
  BOOST_CHECK_EXCEPTION(parse<RuleVO>(toString(inbound)), Exception,
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
