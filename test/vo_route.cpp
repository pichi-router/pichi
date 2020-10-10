#define BOOST_TEST_MODULE pichi vo_route test

#include "utils.hpp"
#include "vo.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/to_json.hpp>

using namespace std;
using namespace rapidjson;

namespace pichi::unit_test {

using vo::parse;
using vo::toJson;
using vo::toString;

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

BOOST_AUTO_TEST_CASE(toJson_Route_Empty)
{
  auto rvo = vo::Route{};
  BOOST_CHECK_EXCEPTION(toJson(rvo, alloc), Exception, verifyException<PichiError::MISC>);

  rvo.default_ = "";
  BOOST_CHECK_EXCEPTION(toJson(rvo, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Route_Without_Rules)
{
  auto expect = Value{};
  auto array = Value{};

  expect.SetObject();
  array.SetArray();
  expect.AddMember(vo::route::DEFAULT, ph, alloc);
  expect.AddMember(vo::route::RULES, array, alloc);

  auto rvo = vo::Route{ph};
  BOOST_CHECK(expect == toJson(rvo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Route_With_Rules)
{
  auto expect = Value{};
  auto rules = Value{};
  auto rule = Value{};

  expect.SetObject();
  rules.SetArray();
  rule.SetArray();
  expect.AddMember(vo::route::DEFAULT, ph, alloc);
  expect.AddMember(vo::route::RULES,
                   rules.PushBack(rule.PushBack(ph, alloc).PushBack(ph, alloc), alloc), alloc);

  auto rvo = vo::Route{ph, {make_pair(vector<string>{ph}, ph)}};
  BOOST_CHECK(expect == toJson(rvo, alloc));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
