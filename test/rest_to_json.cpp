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

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Invalid_Type)
{
  for (auto t : {AdapterType::DIRECT, AdapterType::REJECT})
    BOOST_CHECK_EXCEPTION(toJson(vo::Ingress{t}, alloc), Exception,
                          verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Default_Ones)
{
  for (auto t : {AdapterType::HTTP, AdapterType::SOCKS5, AdapterType::SS, AdapterType::TUNNEL}) {
    BOOST_CHECK(defaultIngressJson(t) == toJson(defaultIngressVO(t), alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_Mandatory_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto normal = defaultIngressVO(type);
    toJson(normal, alloc);

    auto noBind = normal;
    noBind.bind_.clear();
    BOOST_CHECK_EXCEPTION(toJson(noBind, alloc), Exception, verifyException<PichiError::MISC>);

    auto noPort = normal;
    noPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

    auto noTls = normal;
    noTls.tls_.reset();
    BOOST_CHECK_EXCEPTION(toJson(noTls, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;
    BOOST_CHECK(defaultIngressJson(type) == toJson(vo, alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_TLS_Mandatory_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.tls_ = true;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;

    auto noCertFile = vo;
    noCertFile.certFile_.reset();
    BOOST_CHECK_EXCEPTION(toJson(noCertFile, alloc), Exception, verifyException<PichiError::MISC>);

    auto emptyCertFile = vo;
    emptyCertFile.certFile_->clear();
    BOOST_CHECK_EXCEPTION(toJson(emptyCertFile, alloc), Exception,
                          verifyException<PichiError::MISC>);

    auto noKeyFile = vo;
    noKeyFile.keyFile_.reset();
    BOOST_CHECK_EXCEPTION(toJson(noKeyFile, alloc), Exception, verifyException<PichiError::MISC>);

    auto emptyKeyFile = vo;
    emptyKeyFile.keyFile_->clear();
    BOOST_CHECK_EXCEPTION(toJson(emptyKeyFile, alloc), Exception,
                          verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_TLS_Additional_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.tls_ = true;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;

    auto json = defaultIngressJson(type);
    json[vo::ingress::TLS] = true;
    json.AddMember(vo::ingress::CERT_FILE, ph, alloc);
    json.AddMember(vo::ingress::KEY_FILE, ph, alloc);

    BOOST_CHECK(json == toJson(vo, alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_With_Too_Long_Name)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.credentials_ = unordered_map<string, string>{{string(256_sz, 'n'), ph}};

    BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_With_Too_Long_Password)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.credentials_ = unordered_map<string, string>{{ph, string(256_sz, 'n')}};

    BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_HTTP_SOCKS5_With_Credentials)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultIngressVO(type);
    vo.credentials_ = unordered_map<string, string>{{ph, ph}};

    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(ph, ph, alloc);
    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, credentials, alloc);

    BOOST_CHECK(json == toJson(vo, alloc));
  }
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

BOOST_AUTO_TEST_CASE(toJson_IngressVO_SS_Additional_Fields)
{
  auto vo = defaultIngressVO(AdapterType::SS);
  vo.tls_ = true;
  vo.certFile_ = ph;
  vo.keyFile_ = ph;
  BOOST_CHECK(defaultIngressJson(AdapterType::SS) == toJson(vo, alloc));
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_TUNNEL_Mandatory_Fields)
{
  auto origin = defaultIngressVO(AdapterType::TUNNEL);
  toJson(origin, alloc);

  auto noBind = origin;
  noBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(toJson(noBind, alloc), Exception, verifyException<PichiError::MISC>);

  auto noPort = origin;
  noPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(toJson(noPort, alloc), Exception, verifyException<PichiError::MISC>);

  auto noDest = origin;
  noDest.destinations_.clear();
  BOOST_CHECK_EXCEPTION(toJson(noDest, alloc), Exception, verifyException<PichiError::MISC>);

  auto noBalance = origin;
  noBalance.balance_.reset();
  BOOST_CHECK_EXCEPTION(toJson(noBalance, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Empty_Pack)
{
  auto empty = unordered_map<string, vo::Ingress>{};
  auto expect = Value{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Pack_Empty_Name)
{
  auto src = unordered_map<string, vo::Ingress>{{"", {AdapterType::DIRECT}}};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_IngressVO_Pack)
{
  auto src = unordered_map<string, vo::Ingress>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    expect.AddMember(Value{to_string(i).data(), alloc}, defaultIngressJson(AdapterType::HTTP),
                     alloc);
    src[to_string(i)] = defaultIngressVO(AdapterType::HTTP);
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
  vo.delay_ = 0_u16;
  vo.mode_ = DelayMode::FIXED;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;

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
  vo.delay_ = 301_u16;
  BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Fixed)
{
  for (auto i = 0_u16; i <= 300_u16; ++i) {
    auto vo = defaultEgressVO(AdapterType::REJECT);
    vo.delay_ = i;
    auto expect = defaultEgressJson(AdapterType::REJECT);
    expect[vo::egress::DELAY].SetInt(i);
    auto fact = toJson(vo::Egress{AdapterType::REJECT, {}, {}, {}, {}, DelayMode::FIXED, i}, alloc);
    BOOST_CHECK(expect == fact);
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_REJECT_Random_Additional_Fields)
{
  auto expect = defaultEgressJson(AdapterType::REJECT);
  expect[vo::egress::MODE] = vo::delay::RANDOM;
  expect.RemoveMember(vo::egress::DELAY);

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

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultEgressVO(type);
    vo.delay_ = 0_u16;
    vo.mode_ = DelayMode::FIXED;
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.insecure_ = true;
    vo.caFile_ = ph;

    BOOST_CHECK(defaultEgressJson(type) == toJson(vo, alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS5_Missing_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto origin = defaultEgressVO(type);
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
    emptyPort.port_ = 0_u16;
    BOOST_CHECK_EXCEPTION(toJson(emptyPort, alloc), Exception, verifyException<PichiError::MISC>);

    auto noTls = origin;
    noTls.tls_.reset();
    BOOST_CHECK_EXCEPTION(toJson(noTls, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS5_TLS_Mandatory_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = true;

    auto noInsecure = vo;
    noInsecure.insecure_.reset();
    BOOST_CHECK_EXCEPTION(toJson(noInsecure, alloc), Exception, verifyException<PichiError::MISC>);
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS5_TLS_Addtional_Fields)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = true;
    vo.caFile_ = ph;

    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::INSECURE, true, alloc);

    BOOST_CHECK(json == toJson(vo, alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS5_TLS_With_CA)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    vo.caFile_ = ph;

    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::INSECURE, false, alloc);
    json.AddMember(vo::egress::CA_FILE, ph, alloc);

    BOOST_CHECK(json == toJson(vo, alloc));
  }
}

BOOST_AUTO_TEST_CASE(toJson_Egress_HTTP_SOCKS_TLS_With_Empty_CA)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5}) {
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    vo.caFile_ = "";

    BOOST_CHECK_EXCEPTION(toJson(vo, alloc), Exception, verifyException<PichiError::MISC>);
  }
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
  emptyPort.port_ = 0_u16;
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
  auto empty = unordered_map<string, vo::Egress>{};
  auto expect = Value{};
  expect.SetObject();

  auto fact = toJson(begin(empty), end(empty), alloc);
  BOOST_CHECK(expect == fact);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Pack_Empty_Name)
{
  auto src = unordered_map<string, vo::Egress>{{"", {AdapterType::DIRECT}}};
  BOOST_CHECK_EXCEPTION(toJson(begin(src), end(src), alloc), Exception,
                        verifyException<PichiError::MISC>);
}

BOOST_AUTO_TEST_CASE(toJson_Egress_Pack)
{
  auto src = unordered_map<string, vo::Egress>{};
  auto expect = Value{};
  expect.SetObject();
  for (auto i = 0; i < 10; ++i) {
    expect.AddMember(Value{to_string(i).data(), alloc}, defaultEgressJson(AdapterType::DIRECT),
                     alloc);
    src[to_string(i)] = vo::Egress{AdapterType::DIRECT};
  }

  BOOST_CHECK(expect == toJson(begin(src), end(src), alloc));
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
