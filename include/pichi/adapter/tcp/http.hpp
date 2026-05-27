#ifndef PICHI_ADAPTER_TCP_HTTP_HPP
#define PICHI_ADAPTER_TCP_HTTP_HPP

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <optional>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/concepts.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <string>
#include <variant>

namespace pichi::adapter::tcp {

namespace detail {

using Cache = boost::beast::flat_buffer;
using Body  = boost::beast::http::empty_body;

template <bool isRequest> using Parser  = boost::beast::http::parser<isRequest, Body>;
using RequestParser                     = Parser<true>;
using ResponseParser                    = Parser<false>;
template <bool isRequest> using Header  = boost::beast::http::header<isRequest>;
template <bool isRequest> using Message = boost::beast::http::message<isRequest, Body>;
using Request                           = Message<true>;
using Response                          = Message<false>;

template <stream::AsyncLayer NextLayer> struct InvalidManner {
  Awaitable<size_t> recv(NextLayer&, MutableBuffer);
  Awaitable<void>   send(NextLayer&, ConstBuffer);
  Awaitable<void>   confirm(NextLayer&);
};

template <stream::AsyncLayer NextLayer> class ConnectManner {
public:
  explicit ConnectManner(Cache);

  Awaitable<size_t> recv(NextLayer&, MutableBuffer);
  Awaitable<void>   send(NextLayer&, ConstBuffer);
  Awaitable<void>   confirm(NextLayer&);

private:
  Cache cache_;
};

template <stream::AsyncLayer NextLayer> class ProxyManner {
public:
  explicit ProxyManner(Cache, Request);

  Awaitable<size_t> recv(NextLayer&, MutableBuffer);
  Awaitable<void>   send(NextLayer&, ConstBuffer);
  Awaitable<void>   confirm(NextLayer&);

private:
  Cache in_;
  Cache out_;

  ResponseParser parser_;
};

class HttpIngressCredential {
public:
  explicit HttpIngressCredential(vo::Ingress const&);

  bool authenticate(Request const&) const;

private:
  std::unordered_map<std::string, std::string> data_;
};

class HttpEgressCredential {
public:
  explicit HttpEgressCredential(vo::Egress const&);

  void update(Request&) const;

private:
  std::string data_ = {};
};

}  // namespace detail

template <stream::AsyncLayer NextLayer> class HttpIngress {
private:
  using Manner = std::variant<
      detail::InvalidManner<NextLayer>, detail::ConnectManner<NextLayer>,
      detail::ProxyManner<NextLayer>>;

public:
  explicit HttpIngress(vo::Ingress const&, NextLayer);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  NextLayer underlying_;
  Manner    manner_;

  detail::RequestParser parser_ = {};
  detail::Cache         cache_  = {};

  detail::HttpIngressCredential credential_;
};

template <stream::AsyncLayer NextLayer> class HttpEgress {
private:
  using Credential = std::optional<vo::UpEgressCredential>;

public:
  explicit HttpEgress(vo::Egress const&, NextLayer);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<void> connect(Endpoint const&);

private:
  NextLayer     underlying_;
  Endpoint      peer_;
  detail::Cache cache_;

  detail::HttpEgressCredential credential_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_HTTP_HPP
