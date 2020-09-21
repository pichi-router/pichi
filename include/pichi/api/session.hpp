#ifndef PICHI_API_SESSION_HPP
#define PICHI_API_SESSION_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <memory>
#include <pichi/common/adapter.hpp>
#include <pichi/common/endpoint.hpp>

namespace pichi::api {

class Session : public std::enable_shared_from_this<Session> {
private:
  using Strand = boost::asio::io_context::strand;
  using IngressPtr = std::unique_ptr<Ingress>;
  using EgressPtr = std::unique_ptr<Egress>;

  template <typename Yield> void close(Yield);

public:
  Session(Session const&) = delete;
  Session(Session&&) = delete;
  Session& operator=(Session const&) = delete;
  Session& operator=(Session&&) = delete;

  // According to Effective Moderm C++, Item 22.
  ~Session();
  explicit Session(boost::asio::io_context& io, IngressPtr&&, EgressPtr&&);
  void start(Endpoint const&, Endpoint const&);
  void start(Endpoint const& = {});

private:
  Strand strand_;
  IngressPtr ingress_;
  EgressPtr egress_;
};

}  // namespace pichi::api

#endif
