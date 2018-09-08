#define BOOST_TEST_MODULE pichi rest_to_json test

#include "utils.hpp"
#include <algorithm>
#include <boost/test/unit_test.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/exception.hpp>
#include <string_view>
#include <unordered_map>

using namespace std;
using namespace rapidjson;
using namespace pichi;
using namespace pichi::api;

static decltype(auto) ph = "placeholder";
static auto doc = Document{};
static auto& alloc = doc.GetAllocator();

BOOST_AUTO_TEST_SUITE(REST_TO_JSON)

BOOST_AUTO_TEST_CASE(toJson_AdapterType)
{
  auto map = unordered_map<AdapterType, string_view>{{AdapterType::DIRECT, "direct"},
                                                     {AdapterType::REJECT, "reject"},
                                                     {AdapterType::SOCKS5, "socks5"},
                                                     {AdapterType::HTTP, "http"},
                                                     {AdapterType::SS, "ss"}};
  for_each(begin(map), end(map), [](auto&& pair) {
    auto fact = toJson(pair.first, alloc);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK_EQUAL(pair.second, fact.GetString());
  });
}

BOOST_AUTO_TEST_CASE(toJson_CryptoMethod)
{
  auto map = unordered_map<CryptoMethod, string_view>{
      {CryptoMethod::RC4_MD5, "rc4-md5"},
      {CryptoMethod::BF_CFB, "bf-cfb"},
      {CryptoMethod::AES_128_CTR, "aes-128-ctr"},
      {CryptoMethod::AES_192_CTR, "aes-192-ctr"},
      {CryptoMethod::AES_256_CTR, "aes-256-ctr"},
      {CryptoMethod::AES_128_CFB, "aes-128-cfb"},
      {CryptoMethod::AES_192_CFB, "aes-192-cfb"},
      {CryptoMethod::AES_256_CFB, "aes-256-cfb"},
      {CryptoMethod::CAMELLIA_128_CFB, "camellia-128-cfb"},
      {CryptoMethod::CAMELLIA_192_CFB, "camellia-192-cfb"},
      {CryptoMethod::CAMELLIA_256_CFB, "camellia-256-cfb"},
      {CryptoMethod::CHACHA20, "chacha20"},
      {CryptoMethod::SALSA20, "salsa20"},
      {CryptoMethod::CHACHA20_IETF, "chacha20-ietf"},
      {CryptoMethod::AES_128_GCM, "aes-128-gcm"},
      {CryptoMethod::AES_192_GCM, "aes-192-gcm"},
      {CryptoMethod::AES_256_GCM, "aes-256-gcm"},
      {CryptoMethod::CHACHA20_IETF_POLY1305, "chacha20-ietf-poly1305"},
      {CryptoMethod::XCHACHA20_IETF_POLY1305, "xchacha20-ietf-poly1305"},
  };
  for_each(begin(map), end(map), [](auto&& pair) {
    auto fact = toJson(pair.first, alloc);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK(pair.second == fact.GetString());
  });
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Direct)
{
  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "direct", alloc);

  auto fact = toJson(IngressVO{AdapterType::DIRECT}, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Direct_Additional_Fields)
{
  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "direct", alloc);

  auto fact = toJson(IngressVO{AdapterType::DIRECT, ph, 0, CryptoMethod::AES_128_CFB, ph}, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Reject)
{
  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "reject", alloc);

  auto fact = toJson(IngressVO{AdapterType::REJECT}, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Reject_Additional_Fields)
{
  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "reject", alloc);

  auto fact = toJson(IngressVO{AdapterType::REJECT, ph, 0, CryptoMethod::AES_128_CFB, ph}, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP)
{
  auto ingress = IngressVO{AdapterType::HTTP, ph, 1};

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "http", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);

  auto fact = toJson(ingress, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_Mandatory_Fields)
{
  auto normal = IngressVO{AdapterType::HTTP, ph, 1};
  toJson(normal, alloc);

  auto noBind = normal;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(toJson(noBind, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = normal;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_Additional_Fields)
{
  auto ingress = IngressVO{AdapterType::HTTP, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(ingress, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "http", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SOCKS5)
{
  auto ingress = IngressVO{AdapterType::SOCKS5, ph, 1};

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "socks5", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);

  auto fact = toJson(ingress, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SOCKS5_Mandatory_Fields)
{
  auto normal = IngressVO{AdapterType::SOCKS5, ph, 1};
  toJson(normal, alloc);

  auto noBind = normal;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(toJson(noBind, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = normal;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SOCKS5_Additional_Fields)
{
  auto ingress = IngressVO{AdapterType::SOCKS5, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(ingress, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "socks5", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SS)
{
  auto ingress = IngressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "ss", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);

  auto fact = toJson(ingress, alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SS_Mandatory_Fields)
{
  auto const origin = IngressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  toJson(origin, alloc);

  auto noBind = origin;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(toJson(noBind, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noMethod, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noPassword, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyPassword, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Empty_Pack)
{
  auto empty = unordered_map<string, IngressVO>{};
  auto expect = Value{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Pack_Empty_Name)
{
  auto src = unordered_map<string, IngressVO>{{"", {AdapterType::DIRECT}}};
  auto doc = Value{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Pack)
{
  auto src = unordered_map<string, IngressVO>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("type", "direct", alloc);
    expect.AddMember(Value{to_string(i).data(), alloc}, v, alloc);
    src[to_string(i)] = IngressVO{AdapterType::DIRECT};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_DIRECT)
{
  auto const origin = EgressVO{AdapterType::DIRECT};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "direct", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_DIRECT_Additional_Fields)
{
  auto const origin = EgressVO{AdapterType::DIRECT, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "direct", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT)
{
  auto const origin = EgressVO{AdapterType::REJECT};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "reject", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Additional_Fields)
{
  auto const origin = EgressVO{AdapterType::REJECT, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "reject", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SOCKS5)
{
  auto const origin = EgressVO{AdapterType::SOCKS5, ph, 1};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "socks5", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SOCKS5_Additional_Fields)
{
  auto const origin = EgressVO{AdapterType::SOCKS5, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "socks5", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SOCKS5_Missing_Fields)
{
  auto const origin = EgressVO{AdapterType::SOCKS5, ph, 1};
  toJson(origin, alloc);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(emptyPort, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP)
{
  auto const origin = EgressVO{AdapterType::HTTP, ph, 1};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "http", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_Additional_Fields)
{
  auto const origin = EgressVO{AdapterType::HTTP, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "http", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_Missing_Fields)
{
  auto const origin = EgressVO{AdapterType::HTTP, ph, 1};
  toJson(origin, alloc);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(emptyPort, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SS)
{
  auto const origin = EgressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("type", "ss", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);
  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SS_Missing_Fields)
{
  auto const origin = EgressVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  toJson(origin, alloc);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyHost, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(emptyPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noMethod, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noPassword, alloc), Exception, verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyPassword, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Empty_Pack)
{
  auto empty = unordered_map<string, EgressVO>{};
  auto expect = Value{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Pack_Empty_Name)
{
  auto src = unordered_map<string, EgressVO>{{"", {AdapterType::DIRECT}}};
  auto doc = Value{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Pack)
{
  auto src = unordered_map<string, EgressVO>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("type", "direct", alloc);
    expect.AddMember(Value{to_string(i).data(), alloc}, v, alloc);
    src[to_string(i)] = EgressVO{AdapterType::DIRECT};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty)
{
  auto const origin = RuleVO{ph};
  toJson(origin, alloc);

  auto emptyEgress = origin;
  emptyEgress.egress_.clear();
  BOOST_CHECK_EXCEPTION(toJson(emptyEgress, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Without_Fields)
{
  auto const origin = RuleVO{ph};
  auto fact = toJson(origin, alloc);

  auto expect = Value{};
  expect.SetObject();
  expect.AddMember("egress", ph, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_With_Fields)
{
  auto const origin = RuleVO{ph};
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Value{};
    auto array = Value{};
    expect.SetObject();
    expect.AddMember("egress", ph, alloc);
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return expect;
  };

  auto range = origin;
  range.range_.emplace_back(ph);
  BOOST_CHECK(generate("range", ph) == toJson(range, alloc));

  auto ingress = origin;
  ingress.ingress_.emplace_back(ph);
  BOOST_CHECK(generate("ingress_name", ph) == toJson(ingress, alloc));

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  BOOST_CHECK(generate("ingress_type", AdapterType::DIRECT) == toJson(type, alloc));

  auto pattern = origin;
  pattern.pattern_.emplace_back(ph);
  BOOST_CHECK(generate("pattern", ph) == toJson(pattern, alloc));

  auto domain = origin;
  domain.domain_.emplace_back(ph);
  BOOST_CHECK(generate("domain", ph) == toJson(domain, alloc));

  auto country = origin;
  country.country_.emplace_back(ph);
  BOOST_CHECK(generate("country", ph) == toJson(country, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty_Pack)
{
  auto empty = unordered_map<string, RuleVO>{};
  auto expect = Value{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack_Empty_Name)
{
  auto src = unordered_map<string, RuleVO>{{"", {ph}}};
  auto doc = Value{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack)
{
  auto src = unordered_map<string, RuleVO>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("egress", ph, alloc);
    expect.AddMember(Value{to_string(i).data(), alloc}, v, alloc);
    src[to_string(i)] = RuleVO{ph};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Route_Empty)
{
  auto rvo = RouteVO{};
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
  expect.AddMember("default", ph, alloc);
  expect.AddMember("rules", array, alloc);

  auto rvo = RouteVO{ph};
  BOOST_CHECK(expect == toJson(rvo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Route_With_Rules)
{
  auto expect = Value{};
  auto array = Value{};

  expect.SetObject();
  array.SetArray();
  expect.AddMember("default", ph, alloc);
  expect.AddMember("rules", array.PushBack(ph, alloc), alloc);

  auto rvo = RouteVO{ph, {ph}};
  BOOST_CHECK(expect == toJson(rvo, alloc));
}

BOOST_AUTO_TEST_SUITE_END()
