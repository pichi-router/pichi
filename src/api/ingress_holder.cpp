#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/io_context.hpp>
#include <pichi/api/balancer.hpp>
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

static any createData(vo::Ingress const& vo)
{
  switch (vo.type_) {
  case AdapterType::TUNNEL: {
    auto& opt = get<vo::TunnelOption>(*vo.opt_);
    return make_shared<Balancer>(opt.balance_, cbegin(opt.destinations_), cend(opt.destinations_));
  }
  default:
    return {};
  }
}

IngressHolder::IngressHolder(asio::io_context& io, vo::Ingress&& vo)
  : vo_{move(vo)}, acceptors_{createAcceptors(io, vo_)}, data_{createData(vo_)}
{
}

void IngressHolder::reset(asio::io_context& io, vo::Ingress&& vo)
{
  // TODO Exception safety
  auto acceptors = createAcceptors(io, vo);
  data_ = createData(vo);
  swap(acceptors_, acceptors);
  vo_ = move(vo);
}

}  // namespace pichi::api
