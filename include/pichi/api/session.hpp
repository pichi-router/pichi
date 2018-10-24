#ifndef PICHI_API_SESSION_HPP
#define PICHI_API_SESSION_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

namespace pichi::net {

class Endpoint;
class Ingress;
class Egress;

} // namespace pichi::net

namespace pichi::api {

class Session : public std::enable_shared_from_this<Session> {
private:
  using Strand = boost::asio::io_context::strand;
  using IngressPtr = std::unique_ptr<net::Ingress>;
  using EgressPtr = std::unique_ptr<net::Egress>;

  template <typename Function> void spawn(Function&&);

public:
  explicit Session(boost::asio::io_context& io, IngressPtr&&, EgressPtr&&);
  void start(net::Endpoint const&, net::Endpoint const&);

private:
  Strand strand_;
  IngressPtr ingress_;
  EgressPtr egress_;
};

} // namespace pichi::api

#endif
