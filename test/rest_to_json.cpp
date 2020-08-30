#define BOOST_TEST_MODULE pichi rest_to_json test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/common/exception.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>
#include <pichi/vo/to_json.hpp>
#include <string_view>
#include <unordered_map>

using namespace std;
using namespace rapidjson;
using namespace pichi;

using vo::toJson;

BOOST_AUTO_TEST_SUITE(REST_TO_JSON)

BOOST_AUTO_TEST_CASE(toJson_AdapterType)
{
  auto map = unordered_map<AdapterType, string_view>{{AdapterType::DIRECT, vo::type::DIRECT},
                                                     {AdapterType::REJECT, vo::type::REJECT},
                                                     {AdapterType::SOCKS5, vo::type::SOCKS5},
                                                     {AdapterType::HTTP, vo::type::HTTP},
                                                     {AdapterType::SS, vo::type::SS}};
  for_each(begin(map), end(map), [](auto&& pair) {
    auto fact = toJson(pair.first, alloc);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK_EQUAL(pair.second, fact.GetString());
  });
}

BOOST_AUTO_TEST_CASE(toJson_CryptoMethod)
{
  auto map = unordered_map<CryptoMethod, string_view>{
      {CryptoMethod::RC4_MD5, vo::method::RC4_MD5},
      {CryptoMethod::BF_CFB, vo::method::BF_CFB},
      {CryptoMethod::AES_128_CTR, vo::method::AES_128_CTR},
      {CryptoMethod::AES_192_CTR, vo::method::AES_192_CTR},
      {CryptoMethod::AES_256_CTR, vo::method::AES_256_CTR},
      {CryptoMethod::AES_128_CFB, vo::method::AES_128_CFB},
      {CryptoMethod::AES_192_CFB, vo::method::AES_192_CFB},
      {CryptoMethod::AES_256_CFB, vo::method::AES_256_CFB},
      {CryptoMethod::CAMELLIA_128_CFB, vo::method::CAMELLIA_128_CFB},
      {CryptoMethod::CAMELLIA_192_CFB, vo::method::CAMELLIA_192_CFB},
      {CryptoMethod::CAMELLIA_256_CFB, vo::method::CAMELLIA_256_CFB},
      {CryptoMethod::CHACHA20, vo::method::CHACHA20},
      {CryptoMethod::SALSA20, vo::method::SALSA20},
      {CryptoMethod::CHACHA20_IETF, vo::method::CHACHA20_IETF},
      {CryptoMethod::AES_128_GCM, vo::method::AES_128_GCM},
      {CryptoMethod::AES_192_GCM, vo::method::AES_192_GCM},
      {CryptoMethod::AES_256_GCM, vo::method::AES_256_GCM},
      {CryptoMethod::CHACHA20_IETF_POLY1305, vo::method::CHACHA20_IETF_POLY1305},
      {CryptoMethod::XCHACHA20_IETF_POLY1305, vo::method::XCHACHA20_IETF_POLY1305},
  };
  for_each(begin(map), end(map), [](auto&& pair) {
    auto fact = toJson(pair.first, alloc);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK(pair.second == fact.GetString());
  });
}

BOOST_AUTO_TEST_CASE(toJson_Balance)
{
  for (auto p : {make_pair(BalanceType::RANDOM, vo::balance::RANDOM),
                 make_pair(BalanceType::ROUND_ROBIN, vo::balance::ROUND_ROBIN),
                 make_pair(BalanceType::LEAST_CONN, vo::balance::LEAST_CONN)}) {
    auto&& [v, expect] = p;
    auto fact = toJson(v, alloc);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK_EQUAL(expect, fact.GetString());
  }
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
