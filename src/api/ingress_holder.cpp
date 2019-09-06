#include <boost/asio/io_context.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;
namespace ip = asio::ip;

namespace pichi::api {

IngressHolder::IngressHolder(asio::io_context& io, IngressVO&& vo)
  : vo_{move(vo)}, balancer_{vo_.type_ == AdapterType::TUNNEL ?
                                 makeBalancer(*vo_.balance_, cbegin(vo_.destinations_),
                                              cend(vo_.destinations_)) :
                                 nullptr},
    acceptor_{io, {ip::make_address(vo_.bind_), vo_.port_}}
{
}

void IngressHolder::reset(asio::io_context& io, IngressVO&& vo)
{
  vo_ = move(vo);
  balancer_ = vo_.type_ == AdapterType::TUNNEL ?
                  makeBalancer(*vo_.balance_, cbegin(vo_.destinations_), cend(vo_.destinations_)) :
                  nullptr;
  acceptor_ = ip::tcp::acceptor{io, {ip::make_address(vo_.bind_), vo_.port_}};
}

} // namespace pichi::api
