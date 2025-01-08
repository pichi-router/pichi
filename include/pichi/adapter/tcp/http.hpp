#ifndef PICHI_ADAPTER_TCP_HTTP_HPP
#define PICHI_ADAPTER_TCP_HTTP_HPP

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <functional>
#include <optional>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
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

struct InvalidManner {
  template <typename Stream> Awaitable<size_t> recv(Stream&, MutableBuffer);
  template <typename Stream> Awaitable<void>   send(Stream&, ConstBuffer);
  template <typename Stream> Awaitable<void>   confirm(Stream&);
};

class ConnectManner {
public:
  explicit ConnectManner(Cache);

  template <typename Stream> Awaitable<size_t> recv(Stream&, MutableBuffer);
  template <typename Stream> Awaitable<void>   send(Stream&, ConstBuffer);
  template <typename Stream> Awaitable<void>   confirm(Stream&);

private:
  Cache cache_;
};

class ProxyManner {
public:
  explicit ProxyManner(Cache, Request);

  template <typename Stream> Awaitable<size_t> recv(Stream&, MutableBuffer);
  template <typename Stream> Awaitable<void>   send(Stream&, ConstBuffer);
  template <typename Stream> Awaitable<void>   confirm(Stream&);

private:
  Cache   cache_;
  Request req_;
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

template <typename Socket> class HttpIngress {
private:
  using Manner = std::variant<detail::InvalidManner, detail::ConnectManner, detail::ProxyManner>;
  using Stream = std::variant<stream::Tls<Socket>, Socket>;

public:
  explicit HttpIngress(vo::Ingress const&, Socket);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  Stream stream_;
  Manner manner_ = {};

  detail::RequestParser parser_ = {};
  detail::Cache         cache_  = {};

  detail::HttpIngressCredential credential_;
};

template <typename Socket> class HttpEgress {
private:
  using Credential = std::optional<vo::UpEgressCredential>;
  using Stream     = std::variant<stream::Tls<Socket>, Socket>;

public:
  explicit HttpEgress(vo::Egress const&, IOExecutor const&);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<void> connect(Endpoint const&);

private:
  Stream        stream_;
  Endpoint      peer_;
  detail::Cache cache_;

  detail::HttpEgressCredential credential_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_HTTP_HPP
