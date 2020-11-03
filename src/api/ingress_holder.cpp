#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/io_context.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/vo/ingress.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;

namespace pichi::api {

static auto createAcceptors(asio::io_context& io, vo::Ingress const& vo)
{
  auto acceptors = vector<ip::tcp::acceptor>{};
  transform(cbegin(vo.bind_), cend(vo.bind_), back_inserter(acceptors), [&io](auto&& endpoint) {
    assertFalse(endpoint.type_ == EndpointType::DOMAIN_NAME);
    return ip::tcp::acceptor{io, {ip::make_address(endpoint.host_), endpoint.port_}};
  });
  return acceptors;
}

IngressHolder::IngressHolder(asio::io_context& io, vo::Ingress&& vo)
  : vo_{move(vo)},
    balancer_{vo_.type_ == AdapterType::TUNNEL
                  ? makeBalancer(get<vo::TunnelOption>(*vo_.opt_).balance_,
                                 cbegin(get<vo::TunnelOption>(*vo_.opt_).destinations_),
                                 cend(get<vo::TunnelOption>(*vo_.opt_).destinations_))
                  : nullptr},
    acceptors_{createAcceptors(io, vo_)}
{
}

void IngressHolder::reset(asio::io_context& io, vo::Ingress&& vo)
{
  // TODO Exception safety
  vo_ = move(vo);
  balancer_ = vo_.type_ == AdapterType::TUNNEL
                  ? makeBalancer(get<vo::TunnelOption>(*vo_.opt_).balance_,
                                 cbegin(get<vo::TunnelOption>(*vo_.opt_).destinations_),
                                 cend(get<vo::TunnelOption>(*vo_.opt_).destinations_))
                  : nullptr;
  auto acceptors = createAcceptors(io, vo_);
  swap(acceptors_, acceptors);
}

}  // namespace pichi::api
