#ifndef PICHI_API_INGRESS_HOLDER_HPP
#define PICHI_API_INGRESS_HOLDER_HPP

#include <any>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/api/balancer.hpp>
#include <pichi/vo/ingress.hpp>
#include <vector>

namespace boost::asio {

class io_context;

}  // namespace boost::asio

namespace pichi::api {

struct IngressHolder {
  explicit IngressHolder(boost::asio::io_context&, vo::Ingress&&);
  ~IngressHolder() = default;

  IngressHolder(IngressHolder const&) = delete;
  IngressHolder(IngressHolder&&) = delete;
  IngressHolder& operator=(IngressHolder const&) = delete;
  IngressHolder& operator=(IngressHolder&&) = delete;

  void reset(boost::asio::io_context&, vo::Ingress&&);

  vo::Ingress vo_;
  std::vector<boost::asio::ip::tcp::acceptor> acceptors_;
  std::any data_;
};

}  // namespace pichi::api

#endif  // PICHI_API_INGRESS_HOLDER_HPP
