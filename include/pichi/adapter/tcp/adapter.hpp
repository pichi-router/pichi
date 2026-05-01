#ifndef PICHI_ADAPTER_TCP_ADAPTER_HPP
#define PICHI_ADAPTER_TCP_ADAPTER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/adapter/tcp/direct.hpp>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/adapter/tcp/reject.hpp>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/adapter/tcp/trojan.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/stream/concepts.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <variant>

namespace pichi::adapter::tcp {

using Ingress = std::variant<
    HttpIngress<boost::asio::ip::tcp::socket>, Socks5Ingress<boost::asio::ip::tcp::socket>,
    TrojanIngress<boost::asio::ip::tcp::socket>, Shadowsocks<boost::asio::ip::tcp::socket>>;

using Egress = std::variant<
    Direct, RejectEgress, HttpEgress<boost::asio::ip::tcp::socket>,
    Socks5Egress<boost::asio::ip::tcp::socket>, TrojanEgress<boost::asio::ip::tcp::socket>,
    Shadowsocks<boost::asio::ip::tcp::socket>>;

template <stream::AsyncSocket Socket> Ingress create_ingress(vo::Ingress const&, Socket);

extern Egress create_egress(vo::Egress const&, IOExecutor const&);

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_ADAPTER_HPP
