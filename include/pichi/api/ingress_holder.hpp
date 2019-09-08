#ifndef PICHI_API_INGRESS_HOLDER_HPP
#define PICHI_API_INGRESS_HOLDER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <pichi/api/balancer.hpp>
#include <pichi/api/vos.hpp>

namespace boost::asio {

class io_context;

} // namespace boost::asio

namespace pichi::api {

using IngressIterator = decltype(IngressVO::destinations_)::const_iterator;

struct IngressHolder {
  explicit IngressHolder(boost::asio::io_context&, IngressVO&&);
  ~IngressHolder() = default;

  IngressHolder(IngressHolder const&) = delete;
  IngressHolder(IngressHolder&&) = delete;
  IngressHolder& operator=(IngressHolder const&) = delete;
  IngressHolder& operator=(IngressHolder&&) = delete;

  void reset(boost::asio::io_context&, IngressVO&&);

  IngressVO vo_;
  std::unique_ptr<Balancer<IngressIterator>> balancer_;
  boost::asio::ip::tcp::acceptor acceptor_;
};

} // namespace pichi::api

#endif // PICHI_API_INGRESS_HOLDER_HPP
