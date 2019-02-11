#ifndef PICHI_API_SESSION_HPP
#define PICHI_API_SESSION_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#ifndef _MSC_VER

namespace pichi::net {

class Endpoint;
class Ingress;
class Egress;

} // namespace pichi::net

#else // _MSC_VER

// TODO find out why MSVC couldn't compile with the forward declaration
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>

#endif // _MSC_VER

namespace pichi::api {

class Session : public std::enable_shared_from_this<Session> {
private:
  using Strand = boost::asio::io_context::strand;
  using IngressPtr = std::unique_ptr<net::Ingress>;
  using EgressPtr = std::unique_ptr<net::Egress>;

  void close();

public:
  Session(Session const&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session const&) = delete;
  Session& operator=(Session&&) = delete;

  // According to Effective Moderm C++, Item 22.
  ~Session();
  explicit Session(boost::asio::io_context& io, IngressPtr&&, EgressPtr&&);
  void start(net::Endpoint const&, net::Endpoint const&);

private:
  Strand strand_;
  IngressPtr ingress_;
  EgressPtr egress_;
};

} // namespace pichi::api

#endif
