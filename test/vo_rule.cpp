#define BOOST_TEST_MODULE pichi vo_rule test

#include "utils.hpp"
#include "vo.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>
#include <unordered_map>

namespace json = rapidjson;
namespace rngs = std::ranges;

namespace pichi::unit_test {

using vo::parse;
using vo::toJson;
using vo::toString;

template <typename String, rngs::range Range>
void add_member(json::Value& obj, String&& key, Range&& r, json::Document::AllocatorType& alloc)
{
  if (rngs::empty(r)) return;

  auto ret = json::Value{};
  ret.SetArray();
  rngs::for_each(std::forward<Range>(r), [&](auto&& item) {
    ret.PushBack(toJson(item, alloc), alloc);
  });
  obj.AddMember(std::forward<String>(key), std::move(ret), alloc);
}

static std::string to_string(vo::Rule const& rvo)
{
  auto v = json::Value{};
  v.SetObject();

  add_member(v, vo::rule::RANGE, rvo.range_, alloc);
  add_member(v, vo::rule::RANGE_NR, rvo.range_nr_, alloc);
  add_member(v, vo::rule::INGRESS, rvo.ingress_, alloc);
  add_member(v, vo::rule::TYPE, rvo.type_, alloc);
  add_member(v, vo::rule::PATTERN, rvo.pattern_, alloc);
  add_member(v, vo::rule::DOMAIN_NAME, rvo.domain_, alloc);
  add_member(v, vo::rule::COUNTRY, rvo.country_, alloc);
  add_member(v, vo::rule::COUNTRY_NR, rvo.country_nr_, alloc);

  return toString(v);
}

BOOST_AUTO_TEST_SUITE(REST_PARSE)

BOOST_AUTO_TEST_CASE(parse_Rule_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>("not a json"),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>("[\"not a json object\"]"),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE(parse_Rule)
{
  auto const expect = vo::Rule{};
  auto       fact   = parse<vo::Rule>(to_string(expect));
  BOOST_CHECK(fact == expect);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Fields)
{
  auto const origin   = vo::Rule{};
  auto       generate = [](auto&& key, auto&& value) {
    auto expect = json::Document{};
    auto array  = json::Value{};
    expect.SetObject();
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return toString(expect);
  };

  auto range = origin;
  range.range_.emplace_back(ph);
  auto fact = parse<vo::Rule>(generate(vo::rule::RANGE, ph));
  BOOST_CHECK(range == fact);

  auto range_nr = origin;
  range_nr.range_nr_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::RANGE_NR, ph));
  BOOST_CHECK(range_nr == fact);

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

  auto country_nr = origin;
  country_nr.country_nr_.emplace_back(ph);
  fact = parse<vo::Rule>(generate(vo::rule::COUNTRY_NR, ph));
  BOOST_CHECK(country_nr == fact);
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Empty_Fields_Content)
{
  auto const origin = vo::Rule{};
  parse<vo::Rule>(to_string(origin));

  auto range = origin;
  range.range_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(range)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto range_nr = origin;
  range_nr.range_nr_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(range_nr)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto ingress = origin;
  ingress.ingress_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(ingress)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto pattern = origin;
  pattern.pattern_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(pattern)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto domain = origin;
  domain.domain_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(domain)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto country = origin;
  country.country_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(country)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );

  auto country_nr = origin;
  country_nr.country_nr_.emplace_back("");
  BOOST_CHECK_EXCEPTION(
      parse<vo::Rule>(to_string(country_nr)),
      SystemError,
      verify_exception<PichiError::BAD_JSON>
  );
}

BOOST_AUTO_TEST_CASE(parse_Rule_With_Superfluous_Field)
{
  BOOST_CHECK(vo::Rule{} == parse<vo::Rule>("{\"superfluous_field\":\"none\"}"));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Without_Fields)
{
  auto const origin = vo::Rule{};
  auto       fact   = toJson(origin, alloc);

  auto expect = json::Value{};
  expect.SetObject();

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_With_Fields)
{
  auto generate = [](auto&& key, auto&& value) {
    auto expect = json::Value{};
    auto array  = json::Value{};
    expect.SetObject();
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return expect;
  };

  auto range = vo::Rule{};
  range.range_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::RANGE, ph) == toJson(range, alloc));

  auto range_nr = vo::Rule{};
  range_nr.range_nr_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::RANGE_NR, ph) == toJson(range_nr, alloc));

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

  auto country_nr = vo::Rule{};
  country_nr.country_nr_.emplace_back(ph);
  BOOST_CHECK(generate(vo::rule::COUNTRY_NR, ph) == toJson(country_nr, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty_Pack)
{
  auto empty = std::unordered_map<std::string, vo::Rule>{};
  BOOST_CHECK(json::Value{}.SetObject() == toJson(begin(empty), end(empty), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack_Empty_Name)
{
  auto src = std::unordered_map<std::string, vo::Rule>{
      {"", {}}
  };
  BOOST_CHECK_EXCEPTION(
      toJson(begin(src), end(src), alloc),
      SystemError,
      verify_exception<PichiError::MISC>
  );
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack)
{
  auto src    = std::unordered_map<std::string, vo::Rule>{};
  auto expect = json::Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = json::Value{};
    v.SetObject();
    expect.AddMember(json::Value{std::to_string(i).data(), alloc}, v, alloc);
    src[std::to_string(i)] = vo::Rule{};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
