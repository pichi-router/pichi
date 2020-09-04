#define BOOST_TEST_MODULE pichi vo_rule test

#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <rapidjson/document.h>
#include <unordered_map>

using namespace std;
using namespace pichi;
using namespace rapidjson;

using vo::parse;
using vo::toJson;
using vo::toString;

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

BOOST_AUTO_TEST_SUITE(REST_PARSE)

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

BOOST_AUTO_TEST_CASE(toJson_Rule_Without_Fields)
{
  auto const origin = vo::Rule{};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_With_Fields)
{
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Value{};
    auto array = Value{};
    expect.SetObject();
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return expect;
  };

  auto range = vo::Rule{};
  range.range_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::RANGE, ph) == toJson(range, alloc));

  auto ingress = vo::Rule{};
  ingress.ingress_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::INGRESS, ph) == toJson(ingress, alloc));

  auto type = vo::Rule{};
  type.type_.emplace_back(AdapterType::DIRECT);
  BOOST_CHECK(generate(vo::rule::TYPE, AdapterType::DIRECT) == toJson(type, alloc));

  auto pattern = vo::Rule{};
  pattern.pattern_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::PATTERN, ph) == toJson(pattern, alloc));

  auto domain = vo::Rule{};
  domain.domain_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::DOMAIN_NAME, ph) == toJson(domain, alloc));

  auto country = vo::Rule{};
  country.country_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::COUNTRY, ph) == toJson(country, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty_Pack)
{
  auto empty = unordered_map<string, vo::Rule>{};
  BOOST_CHECK(Value{}.SetObject() == toJson(begin(empty), end(empty), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack_Empty_Name)
{
  auto src = unordered_map<string, vo::Rule>{{"", {}}};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack)
{
  auto src = unordered_map<string, vo::Rule>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    expect.AddMember(Value{to_string(i).data(), alloc}, v, alloc);
    src[to_string(i)] = vo::Rule{};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_SUITE_END()
