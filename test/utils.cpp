#include "utils.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <sodium/utils.h>
#include <string.h>

using namespace std;
using namespace pichi::vo;
using namespace rapidjson;

using pichi::AdapterType;

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

vo::Ingress defaultIngressVO(AdapterType type)
{
  auto vo = vo::Ingress{};
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
    vo.destinations_ = {{EndpointType::DOMAIN_NAME, "localhost", "80"}};
    vo.balance_ = BalanceType::RANDOM;
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
  default:
    BOOST_ERROR("Invalid type");
    break;
  }
  return v;
}

Egress defaultEgressVO(AdapterType type)
{
  auto vo = Egress{};
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

}  // namespace pichi
