#define BOOST_TEST_MODULE pichi vo_egress test

#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <rapidjson/document.h>
#include <unordered_map>

using namespace std;
using namespace pichi;
using namespace rapidjson;

using vo::parse;
using vo::toJson;
using vo::toString;

static string toString(vo::Egress const& evo)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember(vo::egress::TYPE, toJson(evo.type_, alloc), alloc);
  if (evo.host_) v.AddMember(vo::egress::HOST, toJson(*evo.host_, alloc), alloc);
  if (evo.port_) v.AddMember(vo::egress::PORT, *evo.port_, alloc);
  if (evo.method_) v.AddMember(vo::egress::METHOD, toJson(*evo.method_, alloc), alloc);
  if (evo.password_) v.AddMember(vo::egress::PASSWORD, toJson(*evo.password_, alloc), alloc);
  if (evo.mode_) v.AddMember(vo::egress::MODE, toJson(*evo.mode_, alloc), alloc);
  if (evo.delay_) v.AddMember(vo::egress::DELAY, *evo.delay_, alloc);
  if (evo.tls_) v.AddMember(vo::egress::TLS, *evo.tls_, alloc);
  if (evo.insecure_) v.AddMember(vo::egress::INSECURE, *evo.insecure_, alloc);
  if (evo.caFile_) v.AddMember(vo::egress::CA_FILE, toJson(*evo.caFile_, alloc), alloc);

  return toString(v);
}

BOOST_AUTO_TEST_SUITE(VO_EGRESS)

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Default_Ones)
{
  for (auto type : {AdapterType::DIRECT, AdapterType::HTTP, AdapterType::REJECT,
                    AdapterType::SOCKS5, AdapterType::SS}) {
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(toString(defaultEgressJson(type))));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_Direct_Additional_Fields)
{
  auto vo = defaultEgressVO(AdapterType::DIRECT);
  vo.delay_ = 0_u16;
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.mode_ = DelayMode::FIXED;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(defaultEgressVO(AdapterType::DIRECT) == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Default_Mode)
{
  auto vo = defaultEgressVO(AdapterType::REJECT);
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.delay_.reset();
  vo.mode_.reset();
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(defaultEgressVO(AdapterType::REJECT) == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Invalid_Mode)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"invalid mode\"}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Random_Additional_Fields)
{
  auto origin = defaultEgressVO(AdapterType::REJECT);
  origin.mode_ = DelayMode::RANDOM;
  origin.delay_.reset();
  BOOST_CHECK(origin == parse<vo::Egress>(toString(origin)));

  auto vo = origin;
  vo.delay_ = 0_u16;
  vo.host_ = ph;
  vo.method_ = CryptoMethod::RC4_MD5;
  vo.password_ = ph;
  vo.port_ = 1_u16;
  vo.tls_ = true;
  vo.insecure_ = true;
  vo.caFile_ = ph;
  BOOST_CHECK(origin == parse<vo::Egress>(toString(vo)));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Missing_Delay)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(
      parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":\"1\"}"), Exception,
      verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Delay_Out_Of_Range)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":-1}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>("{\"type\":\"reject\",\"mode\":\"fixed\",\"delay\":301}"),
                        Exception, verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_Reject_Fixed)
{
  for (auto i = 0_u16; i <= 300_u16; ++i) {
    auto vo = defaultEgressVO(AdapterType::REJECT);
    vo.delay_ = i;
    BOOST_CHECK(vo == parse<vo::Egress>(toString(vo)));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noHost = defaultEgressJson(type);
    noHost.RemoveMember(vo::egress::HOST);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyHost = defaultEgressJson(type);
    emptyHost[vo::egress::HOST] = "";
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyHost)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = defaultEgressJson(type);
    noPort.RemoveMember(vo::egress::PORT);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = defaultEgressJson(type);
    zeroPort[vo::egress::PORT] = 0;
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.AddMember(vo::egress::METHOD, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::PASSWORD, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::MODE, toJson(ph, alloc), alloc);
    json.AddMember(vo::egress::DELAY, 0, alloc);
    json.AddMember(vo::egress::INSECURE, true, alloc);
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_TLS_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json.RemoveMember(vo::egress::TLS);
    BOOST_CHECK(defaultEgressVO(type) == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Default_Insecure_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Insecure_With_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::INSECURE, true, alloc);
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = true;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Secure_With_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
    auto vo = defaultEgressVO(type);
    vo.tls_ = true;
    vo.insecure_ = false;
    vo.caFile_ = ph;
    BOOST_CHECK(vo == parse<vo::Egress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_Secure_With_Empty_CA_Field)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultEgressJson(type);
    json[vo::egress::TLS] = true;
    json.AddMember(vo::egress::CA_FILE, toJson("", alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Incorrect_Type_Credential)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto obj = Value{};
    obj.SetObject();
    auto json = defaultEgressJson(type);
    json.AddMember(vo::egress::CREDENTIAL, obj, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Incorrect_Credential_Pair)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    for (auto count = 0; count < 10; ++count) {
      if (count == 2) continue;
      auto ary = Value{};
      ary.SetArray();
      for (auto i = 0; i < count; ++i) ary.PushBack(ph, alloc);
      auto json = defaultEgressJson(type);
      json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
      BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                            verifyException<PichiError::BAD_JSON>);
    }
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Too_Long_Name)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto ary = Value{};
    ary.SetArray();
    ary.PushBack(toJson(string(256_sz, 'n'), alloc), alloc);
    ary.PushBack(ph, alloc);
    auto json = defaultEgressJson(type);
    json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_HTTP_SOCKS5_With_Too_Long_Password)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto ary = Value{};
    ary.SetArray();
    ary.PushBack(ph, alloc);
    ary.PushBack(toJson(string(256_sz, 'p'), alloc), alloc);
    auto json = defaultEgressJson(type);
    json.AddMember(vo::egress::CREDENTIAL, ary, alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Egress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS_Mandatory_Fields)
{
  auto origin = defaultEgressVO(AdapterType::SS);
  auto holder = parse<vo::Egress>(toString(origin));

  auto noHost = origin;
  noHost.host_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyHost = origin;
  emptyHost.host_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyHost)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPort = origin;
  noPort.port_ = 0_u16;
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_Egress_SS_Additional_Fields)
{
  auto json = defaultEgressJson(AdapterType::SS);
  json.AddMember(vo::egress::MODE, toJson(ph, alloc), alloc);
  json.AddMember(vo::egress::DELAY, 0, alloc);
  json.AddMember(vo::egress::TLS, true, alloc);
  json.AddMember(vo::egress::INSECURE, true, alloc);
  json.AddMember(vo::egress::CA_FILE, toJson(ph, alloc), alloc);
  BOOST_CHECK(defaultEgressVO(AdapterType::SS) == parse<vo::Egress>(json));
}

BOOST_AUTO_TEST_CASE(parse_Egress_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<vo::Egress>(huge), Exception, verifyException<PichiError::BAD_JSON>);
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

BOOST_AUTO_TEST_SUITE_END()
