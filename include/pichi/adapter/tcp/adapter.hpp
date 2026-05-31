#ifndef PICHI_ADAPTER_TCP_ADAPTER_HPP
#define PICHI_ADAPTER_TCP_ADAPTER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/adapter/tcp/direct.hpp>
#include <pichi/adapter/tcp/dual.hpp>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/adapter/tcp/reject.hpp>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/adapter/tcp/transparent.hpp>
#include <pichi/adapter/tcp/trojan.hpp>
#include <pichi/adapter/tcp/tunnel.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/stream/concepts.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/stream/websocket.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <variant>

namespace pichi::adapter::tcp {

using Socket    = boost::asio::ip::tcp::socket;
using Tls       = stream::Tls<Socket>;
using Websocket = stream::Websocket<Tls>;

using Ingress = std::variant<
    DualIngress<Socket>, DualIngress<Tls>, HttpIngress<Socket>, HttpIngress<Tls>,
    Socks5Ingress<Socket>, Socks5Ingress<Tls>, TrojanIngress<Tls>, TrojanIngress<Websocket>,
    Shadowsocks<Socket>, TransparentIngress, Tunnel>;

using Egress = std::variant<
    Direct, RejectEgress, HttpEgress<Socket>, HttpEgress<Tls>, Socks5Egress<Socket>,
    Socks5Egress<Tls>, TrojanEgress<Tls>, TrojanEgress<Websocket>, Shadowsocks<Socket>>;

template <stream::AsyncSocket Socket> Ingress create_ingress(vo::Ingress const&, Socket);

extern Egress create_egress(vo::Egress const&, IOExecutor const&);

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_ADAPTER_HPP
