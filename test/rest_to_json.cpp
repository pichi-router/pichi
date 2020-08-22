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

BOOST_AUTO_TEST_SUITE_END()
