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
static auto serialize = [](auto&& vo) {
  auto doc = Document{};
  return toJson(vo, doc.GetAllocator());
};

BOOST_AUTO_TEST_SUITE(REST_TO_JSON)

BOOST_AUTO_TEST_CASE(toJson_AdapterType)
{
  auto map = unordered_map<AdapterType, string_view>{{AdapterType::DIRECT, "DIRECT"},
                                                     {AdapterType::REJECT, "REJECT"},
                                                     {AdapterType::SOCKS5, "SOCKS5"},
                                                     {AdapterType::HTTP, "HTTP"},
                                                     {AdapterType::SS, "SS"}};
  for_each(begin(map), end(map), [](auto&& pair) {
    auto fact = serialize(pair.first);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK(pair.second == fact.GetString());
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
    auto fact = serialize(pair.first);
    BOOST_CHECK(fact.IsString());
    BOOST_CHECK(pair.second == fact.GetString());
  });
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Direct)
{
  auto expect = Document{};
  expect.SetObject();
  expect.AddMember("type", "DIRECT", expect.GetAllocator());

  auto fact = serialize(InboundVO{AdapterType::DIRECT});
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Direct_Additional_Fields)
{
  auto expect = Document{};
  expect.SetObject();
  expect.AddMember("type", "DIRECT", expect.GetAllocator());

  auto fact = serialize(InboundVO{AdapterType::DIRECT, ph, 0, CryptoMethod::AES_128_CFB, ph});
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Reject)
{
  auto expect = Document{};
  expect.SetObject();
  expect.AddMember("type", "REJECT", expect.GetAllocator());

  auto fact = serialize(InboundVO{AdapterType::REJECT});
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Reject_Additional_Fields)
{
  auto expect = Document{};
  expect.SetObject();
  expect.AddMember("type", "REJECT", expect.GetAllocator());

  auto fact = serialize(InboundVO{AdapterType::REJECT, ph, 0, CryptoMethod::AES_128_CFB, ph});
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_HTTP)
{
  auto ivo = InboundVO{AdapterType::HTTP, ph, 1};

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "HTTP", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);

  auto fact = serialize(ivo);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_HTTP_Mandatory_Fields)
{
  auto normal = InboundVO{AdapterType::HTTP, ph, 1};
  serialize(normal);

  auto noBind = normal;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(serialize(noBind), Exception, verifyException<PichiError::MISC>);

  auto noPort = normal;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_HTTP_Additional_Fields)
{
  auto ivo = InboundVO{AdapterType::HTTP, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(ivo);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "HTTP", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_SOCKS5)
{
  auto ivo = InboundVO{AdapterType::SOCKS5, ph, 1};

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SOCKS5", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);

  auto fact = serialize(ivo);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_SOCKS5_Mandatory_Fields)
{
  auto normal = InboundVO{AdapterType::SOCKS5, ph, 1};
  serialize(normal);

  auto noBind = normal;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(serialize(noBind), Exception, verifyException<PichiError::MISC>);

  auto noPort = normal;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_SOCKS5_Additional_Fields)
{
  auto ivo = InboundVO{AdapterType::SOCKS5, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(ivo);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SOCKS5", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_SS)
{
  auto ivo = InboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SS", alloc);
  expect.AddMember("bind", ph, alloc);
  expect.AddMember("port", 1, alloc);
  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);

  auto fact = serialize(ivo);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_SS_Mandatory_Fields)
{
  auto const origin = InboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  serialize(origin);

  auto noBind = origin;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(serialize(noBind), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noMethod), Exception, verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noPassword), Exception, verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyPassword), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Empty_Pack)
{
  auto empty = unordered_map<string, InboundVO>{};
  auto expect = Document{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), expect.GetAllocator());
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Pack_Empty_Name)
{
  auto src = unordered_map<string, InboundVO>{{"", {AdapterType::DIRECT}}};
  auto doc = Document{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), doc.GetAllocator()), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Inbound_Pack)
{
  auto src = unordered_map<string, InboundVO>{};
  auto expect = Document{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("type", "DIRECT", expect.GetAllocator());
    expect.AddMember(Value{to_string(i).data(), expect.GetAllocator()}, v, expect.GetAllocator());
    src[to_string(i)] = InboundVO{AdapterType::DIRECT};
  }

  auto fact = toJson(begin(src), end(src), expect.GetAllocator());

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_DIRECT)
{
  auto const origin = OutboundVO{AdapterType::DIRECT};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "DIRECT", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_DIRECT_Additional_Fields)
{
  auto const origin = OutboundVO{AdapterType::DIRECT, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "DIRECT", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_REJECT)
{
  auto const origin = OutboundVO{AdapterType::REJECT};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "REJECT", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_REJECT_Additional_Fields)
{
  auto const origin = OutboundVO{AdapterType::REJECT, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "REJECT", alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_SOCKS5)
{
  auto const origin = OutboundVO{AdapterType::SOCKS5, ph, 1};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SOCKS5", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_SOCKS5_Additional_Fields)
{
  auto const origin = OutboundVO{AdapterType::SOCKS5, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SOCKS5", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_SOCKS5_Missing_Fields)
{
  auto const origin = OutboundVO{AdapterType::SOCKS5, ph, 1};
  serialize(origin);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noHost), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyHost), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(emptyPort), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_HTTP)
{
  auto const origin = OutboundVO{AdapterType::HTTP, ph, 1};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "HTTP", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_HTTP_Additional_Fields)
{
  auto const origin = OutboundVO{AdapterType::HTTP, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "HTTP", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_HTTP_Missing_Fields)
{
  auto const origin = OutboundVO{AdapterType::HTTP, ph, 1};
  serialize(origin);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noHost), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyHost), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(emptyPort), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_SS)
{
  auto const origin = OutboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("type", "SS", alloc);
  expect.AddMember("host", ph, alloc);
  expect.AddMember("port", 1, alloc);
  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_SS_Missing_Fields)
{
  auto const origin = OutboundVO{AdapterType::SS, ph, 1, CryptoMethod::AES_128_CFB, ph};
  serialize(origin);

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noHost), Exception, verifyException<PichiError::MISC>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyHost), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noPort), Exception, verifyException<PichiError::MISC>);

  auto emptyPort = origin;
  emptyPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(serialize(emptyPort), Exception, verifyException<PichiError::MISC>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noMethod), Exception, verifyException<PichiError::MISC>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(serialize(noPassword), Exception, verifyException<PichiError::MISC>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyPassword), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_Empty_Pack)
{
  auto empty = unordered_map<string, OutboundVO>{};
  auto expect = Document{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), expect.GetAllocator());
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_Pack_Empty_Name)
{
  auto src = unordered_map<string, OutboundVO>{{"", {AdapterType::DIRECT}}};
  auto doc = Document{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), doc.GetAllocator()), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Outbound_Pack)
{
  auto src = unordered_map<string, OutboundVO>{};
  auto expect = Document{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("type", "DIRECT", expect.GetAllocator());
    expect.AddMember(Value{to_string(i).data(), expect.GetAllocator()}, v, expect.GetAllocator());
    src[to_string(i)] = OutboundVO{AdapterType::DIRECT};
  }

  auto fact = toJson(begin(src), end(src), expect.GetAllocator());

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty)
{
  auto const origin = RuleVO{ph};
  serialize(origin);

  auto emptyOutbound = origin;
  emptyOutbound.outbound_.clear();
  BOOST_CHECK_EXCEPTION(serialize(emptyOutbound), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Without_Fields)
{
  auto const origin = RuleVO{ph};
  auto fact = serialize(origin);

  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  expect.SetObject();
  expect.AddMember("outbound", ph, alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_With_Fields)
{
  auto const origin = RuleVO{ph};
  auto generate = [](auto&& key, auto&& value) {
    auto expect = Document{};
    auto& alloc = expect.GetAllocator();
    auto array = Value{};
    expect.SetObject();
    expect.AddMember("outbound", ph, alloc);
    expect.AddMember(key, array.SetArray().PushBack(toJson(value, alloc), alloc), alloc);
    return expect;
  };

  auto range = origin;
  range.range_.emplace_back(ph);
  BOOST_CHECK(generate("range", ph) == serialize(range));

  auto inbound = origin;
  inbound.inbound_.emplace_back(ph);
  BOOST_CHECK(generate("inbound_name", ph) == serialize(inbound));

  auto type = origin;
  type.type_.emplace_back(AdapterType::DIRECT);
  BOOST_CHECK(generate("inbound_type", AdapterType::DIRECT) == serialize(type));

  auto pattern = origin;
  pattern.pattern_.emplace_back(ph);
  BOOST_CHECK(generate("pattern", ph) == serialize(pattern));

  auto domain = origin;
  domain.domain_.emplace_back(ph);
  BOOST_CHECK(generate("domain", ph) == serialize(domain));

  auto country = origin;
  country.country_.emplace_back(ph);
  BOOST_CHECK(generate("country", ph) == serialize(country));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty_Pack)
{
  auto empty = unordered_map<string, RuleVO>{};
  auto expect = Document{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), expect.GetAllocator());
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack_Empty_Name)
{
  auto src = unordered_map<string, RuleVO>{{"", {ph}}};
  auto doc = Document{};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), doc.GetAllocator()), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack)
{
  auto src = unordered_map<string, RuleVO>{};
  auto expect = Document{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    auto v = Value{};
    v.SetObject();
    v.AddMember("outbound", ph, expect.GetAllocator());
    expect.AddMember(Value{to_string(i).data(), expect.GetAllocator()}, v, expect.GetAllocator());
    src[to_string(i)] = RuleVO{ph};
  }

  auto fact = toJson(begin(src), end(src), expect.GetAllocator());

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Route_Empty)
{
  auto rvo = RouteVO{};
  BOOST_CHECK_EXCEPTION(serialize(rvo), Exception, verifyException<PichiError::MISC>);

  rvo.default_ = "";
  BOOST_CHECK_EXCEPTION(serialize(rvo), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Route_Without_Rules)
{
  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  auto array = Value{};

  expect.SetObject();
  array.SetArray();
  expect.AddMember("default", ph, alloc);
  expect.AddMember("rules", array, alloc);

  auto rvo = RouteVO{ph};
  BOOST_CHECK(expect == serialize(rvo));
}

BOOST_AUTO_TEST_CASE(toJson_Route_With_Rules)
{
  auto expect = Document{};
  auto& alloc = expect.GetAllocator();
  auto array = Value{};

  expect.SetObject();
  array.SetArray();
  expect.AddMember("default", ph, alloc);
  expect.AddMember("rules", array.PushBack(ph, alloc), alloc);

  auto rvo = RouteVO{ph, {ph}};
  BOOST_CHECK(expect == serialize(rvo));
}

BOOST_AUTO_TEST_SUITE_END()
