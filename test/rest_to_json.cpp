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

static IngressVO defaultIngressVO(AdapterType type)
{
  switch (type) {
  case AdapterType::HTTP:
    return {AdapterType::HTTP, ph, 1};
  case AdapterType::SOCKS5:
    return {AdapterType::SOCKS5, ph, 1};
  case AdapterType::SS:
    return {AdapterType::SS, ph, 1, CryptoMethod::RC4_MD5, ph};
  default:
    BOOST_ERROR("Invalid type");
    return {};
  }
}

static auto defaultIngressJson(AdapterType type)
{
  auto v = Value{};
  v.SetObject();
  v.AddMember("bind", ph, alloc);
  v.AddMember("port", 1, alloc);
  switch (type) {
  case AdapterType::HTTP:
    v.AddMember("type", "http", alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    break;
  }
  return v;
}

static EgressVO defaultEgressVO(AdapterType type)
{
  switch (type) {
  case AdapterType::DIRECT:
    return {AdapterType::DIRECT};
  case AdapterType::REJECT:
    return {AdapterType::REJECT, {}, {}, {}, {}, DelayMode::FIXED, 0};
  case AdapterType::HTTP:
    return {AdapterType::HTTP, ph, 1};
  case AdapterType::SOCKS5:
    return {AdapterType::SOCKS5, ph, 1};
  case AdapterType::SS:
    return {AdapterType::SS, ph, 1, CryptoMethod::RC4_MD5, ph};
  default:
    BOOST_ERROR("Invalid type");
    return {};
  }
}

static auto defaultEgressJson(AdapterType type)
{
  auto v = Value{};
  v.SetObject();
  if (type != AdapterType::DIRECT && type != AdapterType::REJECT) {
    v.AddMember("host", ph, alloc);
    v.AddMember("port", 1, alloc);
  }
  switch (type) {
  case AdapterType::DIRECT:
    v.AddMember("type", "direct", alloc);
    break;
  case AdapterType::REJECT:
    v.AddMember("type", "reject", alloc);
    v.AddMember("mode", "fixed", alloc);
    v.AddMember("delay", 0, alloc);
    break;
  case AdapterType::HTTP:
    v.AddMember("type", "http", alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    break;
  }
  return v;
}

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

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Invalid_Type)
{
  for (auto t : {AdapterType::DIRECT, AdapterType::REJECT})
    BOOST_CHECK_EXCEPTION(toJson(IngressVO{t}, alloc), Exception,
                          verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Default_Ones)
{
  for (auto t : {AdapterType::HTTP, AdapterType::SOCKS5, AdapterType::SS})
    BOOST_CHECK(defaultEgressJson(t) == toJson(defaultEgressVO(t), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_Mandatory_Fields)
{
  auto normal = defaultIngressVO(AdapterType::HTTP);
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
  auto ingress = defaultIngressVO(AdapterType::HTTP);
  ingress.method_ = CryptoMethod::AES_128_CFB;
  ingress.password_ = ph;
  auto fact = toJson(ingress, alloc);

  auto expect = defaultIngressJson(AdapterType::HTTP);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SOCKS5_Mandatory_Fields)
{
  auto normal = defaultIngressVO(AdapterType::SOCKS5);
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
  auto ingress = defaultIngressVO(AdapterType::SOCKS5);
  ingress.method_ = CryptoMethod::AES_128_CFB;
  ingress.password_ = ph;
  auto fact = toJson(ingress, alloc);

  auto expect = defaultIngressJson(AdapterType::SOCKS5);
  BOOST_CHECK(expect == fact);

  expect.AddMember("method", "aes-128-cfb", alloc);
  expect.AddMember("password", ph, alloc);
  BOOST_CHECK(expect != fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SS_Mandatory_Fields)
{
  auto origin = defaultIngressVO(AdapterType::SS);
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
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Pack)
{
  auto src = unordered_map<string, IngressVO>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    expect.AddMember(Value{to_string(i).data(), alloc}, defaultIngressJson(AdapterType::HTTP),
                     alloc);
    src[to_string(i)] = IngressVO{AdapterType::HTTP, ph, 1};
  }

  auto fact = toJson(begin(src), end(src), alloc);

  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Default_Ones)
{
  for (auto t : {AdapterType::DIRECT, AdapterType::REJECT, AdapterType::HTTP, AdapterType::SOCKS5,
                 AdapterType::SS})
    BOOST_CHECK(defaultEgressJson(t) == toJson(defaultEgressVO(t), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_DIRECT_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::DIRECT);
  vo.delay_ = 0;
  vo.mode_ = DelayMode::FIXED;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;

  BOOST_CHECK(defaultEgressJson(AdapterType::DIRECT) == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Missing_Mode)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.mode_.reset();
  BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Missing_Delay)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.delay_.reset();
  BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Delay_Out_Of_Range)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.delay_ = 301;
  BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Fixed)
{
  for (auto i = 0; i <= 300; ++i) {
    auto vo = defaultEgressVO(AdapterType::REJECT);
    vo.delay_ = i;
    auto expect = defaultEgressJson(AdapterType::REJECT);
    expect["delay"].SetInt(i);
    auto fact = toJson(EgressVO{AdapterType::REJECT, {}, {}, {}, {}, DelayMode::FIXED, i}, alloc);
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Random_Additional_Fields)
{
  auto expect = defaultEgressJson(AdapterType::REJECT);
  expect["mode"] = "random";
  expect.RemoveMember("delay");

  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.mode_ = DelayMode::RANDOM;
  vo.delay_.reset();

  BOOST_CHECK(expect == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Fixed_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;

  BOOST_CHECK(defaultEgressJson(AdapterType::REJECT) == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SOCKS5_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::SOCKS5);
  vo.delay_ = 0;
  vo.mode_ = DelayMode::FIXED;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;

  BOOST_CHECK(defaultEgressJson(AdapterType::SOCKS5) == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_SOCKS5_Missing_Fields)
{
  auto origin = defaultEgressVO(AdapterType::SOCKS5);
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

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::HTTP);
  vo.delay_ = 0;
  vo.mode_ = DelayMode::FIXED;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  BOOST_CHECK(defaultEgressJson(AdapterType::HTTP) == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_Missing_Fields)
{
  auto origin = defaultEgressVO(AdapterType::HTTP);
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

BOOST_AUTO_TEST_CASE(toJson_Egress_SS_Missing_Fields)
{
  auto origin = defaultEgressVO(AdapterType::SS);
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
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Pack)
{
  auto src = unordered_map<string, EgressVO>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    expect.AddMember(Value{to_string(i).data(), alloc}, defaultEgressJson(AdapterType::DIRECT),
                     alloc);
    src[to_string(i)] = EgressVO{AdapterType::DIRECT};
  }

  BOOST_CHECK(expect == toJson(begin(src), end(src), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Without_Fields)
{
  auto const origin = RuleVO{};
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

  auto range = RuleVO{};
  range.range_.emplace_back(ph);
  BOOST_CHECK(generate("range", ph) == toJson(range, alloc));

  auto ingress = RuleVO{};
  ingress.ingress_.emplace_back(ph);
  BOOST_CHECK(generate("ingress_name", ph) == toJson(ingress, alloc));

  auto type = RuleVO{};
  type.type_.emplace_back(AdapterType::DIRECT);
  BOOST_CHECK(generate("ingress_type", AdapterType::DIRECT) == toJson(type, alloc));

  auto pattern = RuleVO{};
  pattern.pattern_.emplace_back(ph);
  BOOST_CHECK(generate("pattern", ph) == toJson(pattern, alloc));

  auto domain = RuleVO{};
  domain.domain_.emplace_back(ph);
  BOOST_CHECK(generate("domain", ph) == toJson(domain, alloc));

  auto country = RuleVO{};
  country.country_.emplace_back(ph);
  BOOST_CHECK(generate("country", ph) == toJson(country, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Empty_Pack)
{
  auto empty = unordered_map<string, RuleVO>{};
  BOOST_CHECK(Value{}.SetObject() == toJson(begin(empty), end(empty), alloc));
}

BOOST_AUTO_TEST_CASE(toJson_Rule_Pack_Empty_Name)
{
  auto src = unordered_map<string, RuleVO>{{"", {}}};
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
    expect.AddMember(Value{to_string(i).data(), alloc}, v, alloc);
    src[to_string(i)] = RuleVO{};
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
  auto rules = Value{};
  auto rule = Value{};

  expect.SetObject();
  rules.SetArray();
  rule.SetArray();
  expect.AddMember("default", ph, alloc);
  expect.AddMember("rules", rules.PushBack(rule.PushBack(ph, alloc).PushBack(ph, alloc), alloc),
                   alloc);

  auto rvo = RouteVO{ph, {make_pair(ph, ph)}};
  BOOST_CHECK(expect == toJson(rvo, alloc));
}

BOOST_AUTO_TEST_SUITE_END()
