#define BOOST_TEST_MODULE pichi vo_ingress test

#include "utils.hpp"
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/ingress.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <rapidjson/document.h>

using namespace std;
using namespace pichi;
using namespace rapidjson;

using vo::parse;
using vo::toJson;
using vo::toString;

static string toString(vo::Ingress const& ingress)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember(vo::ingress::TYPE, toJson(ingress.type_, alloc), alloc);
  v.AddMember(vo::ingress::BIND, toJson(ingress.bind_, alloc), alloc);
  v.AddMember(vo::ingress::PORT, ingress.port_, alloc);
  if (ingress.method_.has_value())
    v.AddMember(vo::ingress::METHOD, toJson(*ingress.method_, alloc), alloc);
  if (ingress.password_.has_value())
    v.AddMember(vo::ingress::PASSWORD, toJson(*ingress.password_, alloc), alloc);
  if (ingress.tls_.has_value()) v.AddMember(vo::ingress::TLS, *ingress.tls_, alloc);
  if (ingress.certFile_.has_value())
    v.AddMember(vo::ingress::CERT_FILE, toJson(*ingress.certFile_, alloc), alloc);
  if (ingress.keyFile_.has_value())
    v.AddMember(vo::ingress::KEY_FILE, toJson(*ingress.keyFile_, alloc), alloc);
  if (!ingress.destinations_.empty())
    v.AddMember(vo::ingress::DESTINATIONS,
                toJson(begin(ingress.destinations_), end(ingress.destinations_), alloc), alloc);
  if (ingress.balance_.has_value())
    v.AddMember(vo::ingress::BALANCE, toJson(*ingress.balance_, alloc), alloc);

  return toString(v);
}

BOOST_AUTO_TEST_SUITE(VO_INGRESS)

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Str)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("not a json"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("[\"not a json object\"]"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Type)
{
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"invalid\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"direct\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>("{\"type\":\"reject\"}"), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Default_Ones)
{
  for (auto type : {AdapterType::HTTP, AdapterType::SOCKS5, AdapterType::SS, AdapterType::TUNNEL}) {
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(defaultIngressJson(type))));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noType = defaultIngressJson(type);
    noType.RemoveMember(vo::ingress::TYPE);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noType)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noBind = defaultEgressJson(type);
    noBind.RemoveMember(vo::ingress::BIND);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBind)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noPort = defaultEgressJson(type);
    noPort.RemoveMember(vo::ingress::PORT);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto vo = defaultIngressVO(type);
    BOOST_CHECK(vo == parse<vo::Ingress>(toString(vo)));

    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(vo)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Default_TLS_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto json = defaultIngressJson(type);
    json.RemoveMember(vo::ingress::TLS);
    BOOST_CHECK(defaultIngressVO(type) == parse<vo::Ingress>(toString(json)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Empty_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto origin = defaultIngressVO(type);
    parse<vo::Ingress>(toString(origin));

    auto emptyBind = origin;
    emptyBind.bind_.clear();
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyBind)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto zeroPort = origin;
    zeroPort.port_ = 0;
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(zeroPort)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_TLS_Mandatory_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto noCertFile = defaultIngressJson(type);
    noCertFile[vo::ingress::TLS] = true;
    noCertFile.AddMember(vo::ingress::KEY_FILE, toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noCertFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyCertFile = defaultIngressJson(type);
    emptyCertFile[vo::ingress::TLS] = true;
    emptyCertFile.AddMember(vo::ingress::CERT_FILE, toJson("", alloc), alloc);
    emptyCertFile.AddMember(vo::ingress::KEY_FILE, toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyCertFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto noKeyFile = defaultIngressJson(type);
    noKeyFile[vo::ingress::TLS] = true;
    noKeyFile.AddMember(vo::ingress::CERT_FILE, toJson(ph, alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noKeyFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);

    auto emptyKeyFile = defaultIngressJson(type);
    emptyKeyFile[vo::ingress::TLS] = true;
    emptyKeyFile.AddMember(vo::ingress::CERT_FILE, toJson(ph, alloc), alloc);
    emptyKeyFile.AddMember(vo::ingress::KEY_FILE, toJson("", alloc), alloc);
    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyKeyFile)), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_TLS_Additional_Fields)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto vo = defaultIngressVO(type);
    vo.tls_ = true;
    vo.certFile_ = ph;
    vo.keyFile_ = ph;

    auto json = defaultIngressJson(type);
    json[vo::ingress::TLS] = true;
    json.AddMember(vo::ingress::KEY_FILE, toJson(ph, alloc), alloc);
    json.AddMember(vo::ingress::CERT_FILE, toJson(ph, alloc), alloc);
    json.AddMember(vo::ingress::PASSWORD, toJson(ph, alloc), alloc);
    json.AddMember(vo::ingress::METHOD, toJson(ph, alloc), alloc);

    BOOST_CHECK(vo == parse<vo::Ingress>(json));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Incorrect_Type_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto credentials = Value{};
    credentials.SetArray();
    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Empty_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto emptyCred = Value{};
    emptyCred.SetObject();
    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, emptyCred, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Too_Long_Name)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto tooLongName = toJson(string(256_sz, 'n'), alloc);
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(tooLongName, ph, alloc);
    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_With_Too_Long_Password)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto tooLongPassword = toJson(string(256_sz, 'p'), alloc);
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(ph, tooLongPassword, alloc);
    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, credentials, alloc);

    BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(json), Exception,
                          verifyException<PichiError::BAD_JSON>);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_HTTP_SOCKS5_Credentials)
{
  for (auto type : {AdapterType::SOCKS5, AdapterType::HTTP}) {
    auto credentials = Value{};
    credentials.SetObject();
    credentials.AddMember(ph, ph, alloc);

    auto json = defaultIngressJson(type);
    json.AddMember(vo::ingress::CREDENTIALS, credentials, alloc);

    auto ivo = parse<vo::Ingress>(json);
    BOOST_CHECK_EQUAL(1_sz, ivo.credentials_.size());
    auto it = ivo.credentials_.find(ph);
    BOOST_CHECK(it != cend(ivo.credentials_));
    BOOST_CHECK_EQUAL(ph, it->first);
    BOOST_CHECK_EQUAL(ph, it->second);
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Additional_Fields)
{
  auto json = defaultIngressJson(AdapterType::SS);
  json.AddMember(vo::ingress::TLS, false, alloc);
  json.AddMember(vo::ingress::CERT_FILE, toJson(ph, alloc), alloc);
  json.AddMember(vo::ingress::KEY_FILE, toJson(ph, alloc), alloc);
  BOOST_CHECK(defaultIngressVO(AdapterType::SS) == parse<vo::Ingress>(json));
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_SS_Empty_Fields)
{
  auto origin = defaultIngressVO(AdapterType::SS);
  auto holder = parse<vo::Ingress>(toString(origin));

  auto emptyBind = origin;
  emptyBind.bind_.clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyBind)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto zeroPort = origin;
  zeroPort.port_ = 0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(zeroPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noMethod = origin;
  noMethod.method_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noMethod)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPassword = origin;
  noPassword.password_.reset();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto emptyPassword = origin;
  emptyPassword.password_->clear();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(emptyPassword)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Mandatory_Fields)
{
  auto noType = defaultIngressJson(AdapterType::TUNNEL);
  noType.RemoveMember(vo::ingress::TYPE);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noType)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noBind = defaultIngressJson(AdapterType::TUNNEL);
  noBind.RemoveMember(vo::ingress::BIND);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBind)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noPort = defaultIngressJson(AdapterType::TUNNEL);
  noPort.RemoveMember(vo::ingress::PORT);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noPort)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noDest = defaultIngressJson(AdapterType::TUNNEL);
  noDest.RemoveMember(vo::ingress::DESTINATIONS);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noDest)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto noBalance = defaultIngressJson(AdapterType::TUNNEL);
  noBalance.RemoveMember(vo::ingress::BALANCE);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(noBalance)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Empty_Destinations)
{
  auto json = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(json.HasMember(vo::ingress::DESTINATIONS));
  json[vo::ingress::DESTINATIONS].RemoveAllMembers();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(json)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Empty_Host)
{
  auto json = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(json.HasMember(vo::ingress::DESTINATIONS));
  json[vo::ingress::DESTINATIONS].AddMember("", 1, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(json)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Non_Integer_Value)
{
  auto host = "localhost";

  auto bValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(bValue.HasMember(vo::ingress::DESTINATIONS));
  bValue[vo::ingress::DESTINATIONS][host] = true;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(bValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto sValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(sValue.HasMember(vo::ingress::DESTINATIONS));
  sValue[vo::ingress::DESTINATIONS][host] = ph;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(sValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto dValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(dValue.HasMember(vo::ingress::DESTINATIONS));
  dValue[vo::ingress::DESTINATIONS][host] = 0.0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(dValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto oValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(oValue.HasMember(vo::ingress::DESTINATIONS));
  oValue[vo::ingress::DESTINATIONS][host] = Value{}.SetObject();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(oValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto aValue = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(aValue.HasMember(vo::ingress::DESTINATIONS));
  aValue[vo::ingress::DESTINATIONS][host] = Value{}.SetArray();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(aValue)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Destinations_Invalid_Port)
{
  auto negative = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(negative.HasMember(vo::ingress::DESTINATIONS));
  negative[vo::ingress::DESTINATIONS].AddMember(ph, -1, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(negative)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto huge = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(huge.HasMember(vo::ingress::DESTINATIONS));
  huge[vo::ingress::DESTINATIONS].AddMember(ph, 65536, alloc);
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(huge)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Invalid_Balance_Type)
{
  auto b = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(b.HasMember(vo::ingress::BALANCE));
  b[vo::ingress::BALANCE] = true;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(b)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto i = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(b.HasMember(vo::ingress::BALANCE));
  i[vo::ingress::BALANCE] = 0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(i)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto d = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(d.HasMember(vo::ingress::BALANCE));
  d[vo::ingress::BALANCE] = 0.0;
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(d)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto o = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(o.HasMember(vo::ingress::BALANCE));
  o[vo::ingress::BALANCE] = Value{}.SetObject();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(o)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto a = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(a.HasMember(vo::ingress::BALANCE));
  a[vo::ingress::BALANCE] = Value{}.SetArray();
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(a)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Invalid_Balance_String)
{
  auto empty = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(empty.HasMember(vo::ingress::BALANCE));
  empty[vo::ingress::BALANCE] = "";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(empty)), Exception,
                        verifyException<PichiError::BAD_JSON>);

  auto invalid = defaultIngressJson(AdapterType::TUNNEL);
  BOOST_CHECK(invalid.HasMember(vo::ingress::BALANCE));
  invalid[vo::ingress::BALANCE] = "invalid balance";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(toString(invalid)), Exception,
                        verifyException<PichiError::BAD_JSON>);
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_TUNNEL_Balance)
{
  for (auto p : {make_pair(vo::balance::RANDOM, BalanceType::RANDOM),
                 make_pair(vo::balance::ROUND_ROBIN, BalanceType::ROUND_ROBIN),
                 make_pair(vo::balance::LEAST_CONN, BalanceType::LEAST_CONN)}) {
    auto&& [s, v] = p;

    auto json = defaultIngressJson(AdapterType::TUNNEL);
    BOOST_CHECK(json.HasMember(vo::ingress::BALANCE));
    json[vo::ingress::BALANCE].SetString(s, alloc);

    auto vo = defaultIngressVO(AdapterType::TUNNEL);
    vo.balance_ = v;

    BOOST_CHECK(vo == parse<vo::Ingress>(toString(json)));
  }
}

BOOST_AUTO_TEST_CASE(parse_IngressVO_Invalid_Port)
{
  decltype(auto) negative = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":-1}";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(negative), Exception,
                        verifyException<PichiError::BAD_JSON>);

  decltype(auto) huge = "{\"name\":\"p\",\"type\":\"http\",\"bind\":\"p\",\"port\":65536}";
  BOOST_CHECK_EXCEPTION(parse<vo::Ingress>(huge), Exception, verifyException<PichiError::BAD_JSON>);
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

BOOST_AUTO_TEST_SUITE_END()
