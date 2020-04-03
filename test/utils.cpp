#include "utils.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sodium/utils.h>
#include <string.h>

using namespace std;
using namespace pichi::api;
using namespace pichi::net;
using namespace rapidjson;

using pichi::net::AdapterType;

namespace pichi {

static auto doc = Document{};
Document::AllocatorType& alloc = doc.GetAllocator();

static boost::asio::io_context io;
static boost::asio::detail::Pull* pPull = nullptr;
static boost::asio::detail::Push* pPush = nullptr;
static boost::asio::detail::YieldState* pState = nullptr;
boost::asio::yield_context gYield = {io.get_executor(), *pState, *pPush, *pPull};

vector<uint8_t> str2vec(string_view s) { return {cbegin(s), cend(s)}; }

vector<uint8_t> hex2bin(string_view hex)
{
  auto v = vector<uint8_t>(hex.size() / 2, 0);
  sodium_hex2bin(v.data(), v.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
  return v;
}

IngressVO defaultIngressVO(AdapterType type)
{
  auto vo = IngressVO{};
  vo.type_ = type;
  vo.bind_ = ph;
  vo.port_ = 1_u8;
  switch (type) {
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    vo.tls_ = false;
    break;
  case AdapterType::SS:
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    break;
  case AdapterType::TUNNEL:
    vo.destinations_ = {{net::Endpoint::Type::DOMAIN_NAME, "localhost", "80"}};
    vo.balance_ = BalanceType::RANDOM;
    break;
  case AdapterType::TROJAN:
    vo.passwords_ = {ph};
    vo.remote_ = {{net::Endpoint::Type::DOMAIN_NAME, "localhost", "80"}};
    vo.certFile_ = ph;
    vo.keyFile_ = ph;
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return vo;
}

Value defaultIngressJson(AdapterType type)
{
  auto dst = Value{};
  dst.SetObject();
  dst.AddMember("localhost", 80, alloc);
  auto v = Value{};
  v.SetObject();
  v.AddMember("bind", ph, alloc);
  v.AddMember("port", 1, alloc);
  switch (type) {
  case AdapterType::HTTP:
    v.AddMember("type", "http", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  case AdapterType::TUNNEL:
    v.AddMember("type", "tunnel", alloc);
    v.AddMember("destinations", dst, alloc);
    v.AddMember("balance", "random", alloc);
    break;
  case AdapterType::TROJAN:
    v.AddMember("type", "trojan", alloc);
    v.AddMember("passwords", Value{}.SetArray().PushBack(ph, alloc), alloc);
    v.AddMember("remote_host", "localhost", alloc);
    v.AddMember("remote_port", 80, alloc);
    v.AddMember("cert_file", ph, alloc);
    v.AddMember("key_file", ph, alloc);
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

EgressVO defaultEgressVO(AdapterType type)
{
  auto vo = EgressVO{};
  vo.type_ = type;
  switch (type) {
  case AdapterType::DIRECT:
    break;
  case AdapterType::REJECT:
    vo.mode_ = DelayMode::FIXED;
    vo.delay_ = 0_u16;
    break;
  case AdapterType::SS:
    vo.method_ = CryptoMethod::RC4_MD5;
    vo.password_ = ph;
    vo.host_ = ph;
    vo.port_ = 1_u8;
    break;
  case AdapterType::HTTP:
  case AdapterType::SOCKS5:
    vo.host_ = ph;
    vo.port_ = 1_u8;
    vo.tls_ = false;
    break;
  case AdapterType::TROJAN:
    vo.host_ = ph;
    vo.port_ = 1_u8;
    vo.password_ = ph;
    vo.insecure_ = false;
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return vo;
}

Value defaultEgressJson(AdapterType type)
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
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SOCKS5:
    v.AddMember("type", "socks5", alloc);
    v.AddMember("tls", false, alloc);
    break;
  case AdapterType::SS:
    v.AddMember("type", "ss", alloc);
    v.AddMember("method", "rc4-md5", alloc);
    v.AddMember("password", ph, alloc);
    break;
  case AdapterType::TROJAN:
    v.AddMember("type", "trojan", alloc);
    v.AddMember("password", ph, alloc);
    break;
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

bool operator==(Endpoint const& lhs, Endpoint const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_;
}

bool operator==(IngressVO const& lhs, IngressVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.bind_ == rhs.bind_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_ && lhs.tls_ == rhs.tls_ &&
         lhs.certFile_ == rhs.certFile_ && lhs.keyFile_ == rhs.keyFile_ &&
         equal(cbegin(lhs.destinations_), cend(lhs.destinations_), cbegin(rhs.destinations_),
               cend(rhs.destinations_), [](auto&& l, auto&& r) { return l == r; }) &&
         lhs.balance_ == rhs.balance_;
}

bool operator==(EgressVO const& lhs, EgressVO const& rhs)
{
  return lhs.type_ == rhs.type_ && lhs.host_ == rhs.host_ && lhs.port_ == rhs.port_ &&
         lhs.method_ == rhs.method_ && lhs.password_ == rhs.password_ && lhs.mode_ == rhs.mode_ &&
         lhs.delay_ == rhs.delay_ && lhs.tls_ == rhs.tls_ && lhs.insecure_ == rhs.insecure_ &&
         lhs.caFile_ == rhs.caFile_;
}

bool operator==(RuleVO const& lhs, RuleVO const& rhs)
{
  return equal(begin(lhs.range_), end(lhs.range_), begin(rhs.range_), end(rhs.range_)) &&
         equal(begin(lhs.ingress_), end(lhs.ingress_), begin(rhs.ingress_), end(rhs.ingress_)) &&
         equal(begin(lhs.type_), end(lhs.type_), begin(rhs.type_), end(rhs.type_)) &&
         equal(begin(lhs.pattern_), end(lhs.pattern_), begin(rhs.pattern_), end(rhs.pattern_)) &&
         equal(begin(lhs.domain_), end(lhs.domain_), begin(rhs.domain_), end(rhs.domain_)) &&
         equal(begin(lhs.country_), end(lhs.country_), begin(rhs.country_), end(rhs.country_));
}

bool operator==(RouteVO const& lhs, RouteVO const& rhs)
{
  return lhs.default_ == rhs.default_ &&
         equal(begin(lhs.rules_), end(lhs.rules_), begin(rhs.rules_), end(rhs.rules_));
}

string toString(Value const& v)
{
  auto buf = StringBuffer{};
  auto writer = Writer<StringBuffer>{buf};
  v.Accept(writer);
  return buf.GetString();
}

string toString(IngressVO const& ingress)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember("type", toJson(ingress.type_, alloc), alloc);
  v.AddMember("bind", toJson(ingress.bind_, alloc), alloc);
  v.AddMember("port", ingress.port_, alloc);
  if (ingress.method_.has_value()) v.AddMember("method", toJson(*ingress.method_, alloc), alloc);
  if (ingress.password_.has_value())
    v.AddMember("password", toJson(*ingress.password_, alloc), alloc);
  if (ingress.tls_.has_value()) v.AddMember("tls", *ingress.tls_, alloc);
  if (ingress.certFile_.has_value())
    v.AddMember("cert_file", toJson(*ingress.certFile_, alloc), alloc);
  if (ingress.keyFile_.has_value())
    v.AddMember("key_file", toJson(*ingress.keyFile_, alloc), alloc);
  if (!ingress.destinations_.empty())
    v.AddMember("destinations",
                toJson(begin(ingress.destinations_), end(ingress.destinations_), alloc), alloc);
  if (ingress.balance_.has_value()) v.AddMember("balance", toJson(*ingress.balance_, alloc), alloc);

  return toString(v);
}

string toString(EgressVO const& evo)
{
  auto v = Value{};
  v.SetObject();

  v.AddMember("type", toJson(evo.type_, alloc), alloc);
  if (evo.host_) v.AddMember("host", toJson(*evo.host_, alloc), alloc);
  if (evo.port_) v.AddMember("port", *evo.port_, alloc);
  if (evo.method_) v.AddMember("method", toJson(*evo.method_, alloc), alloc);
  if (evo.password_) v.AddMember("password", toJson(*evo.password_, alloc), alloc);
  if (evo.mode_) v.AddMember("mode", toJson(*evo.mode_, alloc), alloc);
  if (evo.delay_) v.AddMember("delay", *evo.delay_, alloc);
  if (evo.tls_) v.AddMember("tls", *evo.tls_, alloc);
  if (evo.insecure_) v.AddMember("insecure", *evo.insecure_, alloc);
  if (evo.caFile_) v.AddMember("ca_file", toJson(*evo.caFile_, alloc), alloc);

  return toString(v);
}

string toString(RuleVO const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (!rvo.range_.empty())
    v.AddMember("range", toJson(begin(rvo.range_), end(rvo.range_), alloc), alloc);
  if (!rvo.ingress_.empty())
    v.AddMember("ingress_name", toJson(begin(rvo.ingress_), end(rvo.ingress_), alloc), alloc);
  if (!rvo.type_.empty())
    v.AddMember("ingress_type", toJson(begin(rvo.type_), end(rvo.type_), alloc), alloc);
  if (!rvo.pattern_.empty())
    v.AddMember("pattern", toJson(begin(rvo.pattern_), end(rvo.pattern_), alloc), alloc);
  if (!rvo.domain_.empty())
    v.AddMember("domain", toJson(begin(rvo.domain_), end(rvo.domain_), alloc), alloc);
  if (!rvo.country_.empty())
    v.AddMember("country", toJson(begin(rvo.country_), end(rvo.country_), alloc), alloc);

  return toString(v);
}

string toString(RouteVO const& rvo)
{
  auto v = Value{};
  v.SetObject();

  if (rvo.default_) v.AddMember("default", toJson(*rvo.default_, alloc), alloc);
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
  v.AddMember("rules", rules, alloc);

  return toString(v);
}

} // namespace pichi
