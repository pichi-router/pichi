#include "pichi/common/config.hpp"
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/enumerations.hpp>
#include <utility>

using namespace std::literals;

namespace pichi::adapter::tcp {

template <stream::AsyncSocket Socket> Ingress create_ingress(vo::Ingress const& vo, Socket s)
{
  switch (vo.type_) {
  case AdapterType::SS:
    return Ingress{std::in_place_type<Shadowsocks<Socket>>, vo, std::move(s)};
  case AdapterType::SOCKS5:
    if (vo.tls_.has_value())
      return Ingress{
          std::in_place_type<Socks5Ingress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_), std::move(s)}
      };
    else
      return Ingress{std::in_place_type<Socks5Ingress<Socket>>, vo, std::move(s)};
  case AdapterType::HTTP:
    if (vo.tls_.has_value())
      return Ingress{
          std::in_place_type<HttpIngress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_), std::move(s)}
      };
    else
      return Ingress{std::in_place_type<HttpIngress<Socket>>, vo, std::move(s)};
  case AdapterType::DUAL:
    if (vo.tls_.has_value())
      return Ingress{
          std::in_place_type<DualIngress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_), std::move(s)}
      };
    else
      return Ingress{std::in_place_type<DualIngress<Socket>>, vo, std::move(s)};
  case AdapterType::TROJAN:
    if (vo.websocket_.has_value())
      return Ingress{
          std::in_place_type<TrojanIngress<Websocket>>,
          vo,
          Websocket{
                    vo.websocket_->path_,
                    vo.websocket_->host_.value_or(""),
                    stream::tls_context(*vo.tls_),
                    std::move(s)
          }
      };
    else
      return Ingress{
          std::in_place_type<TrojanIngress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_), std::move(s)}
      };
  case AdapterType::TRANSPARENT:
    return Ingress{std::in_place_type<TransparentIngress>, vo, std::move(s)};
  case AdapterType::TUNNEL:
    return Ingress{std::in_place_type<Tunnel>, vo, std::move(s)};
  default:
    fail();
  }
}

Egress create_egress(vo::Egress const& vo, IOExecutor const& ex)
{
  switch (vo.type_) {
  case AdapterType::SS:
    return Egress{std::in_place_type<Shadowsocks<Socket>>, vo, ex};
  case AdapterType::DIRECT:
    return Egress{std::in_place_type<Direct>, ex};
  case AdapterType::REJECT:
    return Egress{std::in_place_type<RejectEgress>, vo, ex};
  case AdapterType::SOCKS5:
    if (vo.tls_.has_value()) {
      return Egress{
          std::in_place_type<Socks5Egress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_, vo.server_->host_), Socket{ex}}
      };
    }
    else
      return Egress{std::in_place_type<Socks5Egress<Socket>>, vo, Socket{ex}};
  case AdapterType::HTTP:
    if (vo.tls_.has_value()) {
      return Egress{
          std::in_place_type<HttpEgress<Tls>>,
          vo,
          Tls{stream::tls_context(*vo.tls_, vo.server_->host_), Socket{ex}}
      };
    }
    else
      return Egress{std::in_place_type<HttpEgress<Socket>>, vo, Socket{ex}};
  case AdapterType::TROJAN:
    if (vo.websocket_.has_value())
      return Egress{
          std::in_place_type<TrojanEgress<Websocket>>,
          vo,
          Websocket{
                    vo.websocket_->path_,
                    vo.websocket_->host_.value_or(""),
                    vo.tls_->sni_,
                    stream::tls_context(*vo.tls_, vo.server_->host_),
                    ex
          }
      };
    else
      return Egress{
          std::in_place_type<TrojanEgress<Tls>>,
          vo,
          Tls{vo.tls_->sni_, stream::tls_context(*vo.tls_, vo.server_->host_), ex}
      };
  default:
    fail();
  }
}

template Ingress create_ingress(vo::Ingress const&, Socket);

}  // namespace pichi::adapter::tcp
