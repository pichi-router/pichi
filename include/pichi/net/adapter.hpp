#ifndef PICHI_NET_ADAPTER_HPP
#define PICHI_NET_ADAPTER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <pichi/common/adapter.hpp>

namespace pichi::vo {

struct Ingress;
struct Egress;

}  // namespace pichi::vo

namespace pichi::api {

struct IngressHolder;

}  // namespace pichi::api

namespace pichi::net {

std::unique_ptr<Ingress> makeIngress(api::IngressHolder&, boost::asio::ip::tcp::socket&&);
std::unique_ptr<Egress> makeEgress(vo::Egress const&, boost::asio::io_context&);

}  // namespace pichi::net

#endif  // PICHI_NET_ADAPTER_HPP
