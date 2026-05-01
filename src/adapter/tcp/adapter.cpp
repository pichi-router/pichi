#include "pichi/common/config.hpp"
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/enumerations.hpp>
#include <utility>

namespace asio = boost::asio;
namespace ip   = asio::ip;

using namespace std::literals;

namespace pichi::adapter::tcp {

template <stream::AsyncSocket Socket> Ingress create_ingress(vo::Ingress const& vo, Socket s)
{
  switch (vo.type_) {
  case AdapterType::SS:
    return Ingress{std::in_place_type<Shadowsocks<Socket>>, vo, std::move(s)};
  case AdapterType::SOCKS5:
    return Ingress{std::in_place_type<Socks5Ingress<Socket>>, vo, std::move(s)};
  case AdapterType::HTTP:
    return Ingress{std::in_place_type<HttpIngress<Socket>>, vo, std::move(s)};
  case AdapterType::TROJAN:
    return Ingress{std::in_place_type<TrojanIngress<Socket>>, vo, std::move(s)};
  default:
    fail();
  }
}

Egress create_egress(vo::Egress const& vo, IOExecutor const& ex)
{
  switch (vo.type_) {
  case AdapterType::SS:
    return Egress{std::in_place_type<Shadowsocks<ip::tcp::socket>>, vo, ex};
  case AdapterType::DIRECT:
    return Egress{std::in_place_type<Direct>, ex};
  case AdapterType::REJECT:
    return Egress{std::in_place_type<RejectEgress>, vo, ex};
  case AdapterType::SOCKS5:
    return Egress{std::in_place_type<Socks5Egress<ip::tcp::socket>>, vo, ex};
  case AdapterType::HTTP:
    return Egress{std::in_place_type<HttpEgress<ip::tcp::socket>>, vo, ex};
  case AdapterType::TROJAN:
    return Egress{std::in_place_type<TrojanEgress<ip::tcp::socket>>, vo, ex};
  default:
    fail();
  }
}

template Ingress create_ingress(vo::Ingress const&, ip::tcp::socket);

}  // namespace pichi::adapter::tcp
