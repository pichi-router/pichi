#ifndef PICHI_NET_HTTP_HPP
#define PICHI_NET_HTTP_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <pichi/buffer.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <utility>

namespace pichi::net {

namespace detail {

using Socket = boost::asio::ip::tcp::socket;
using Yield = boost::asio::yield_context;

using Cache = boost::beast::flat_buffer;
using Body = boost::beast::http::empty_body;

template <bool isRequest> using Parser = boost::beast::http::parser<isRequest, Body>;
using RequestParser = Parser<true>;
using ResponseParser = Parser<false>;

} // namespace detail

class HttpIngress : public Ingress {
public:
  HttpIngress(detail::Socket&&);
  ~HttpIngress() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;

  void send(ConstBuffer<uint8_t>, Yield) override;

  void close() override;

  bool readable() const override;

  bool writable() const override;

  void confirm(Yield yield) override;

  void disconnect(Yield) override;

  Endpoint readRemote(Yield) override;

private:
  detail::Socket socket_;
  detail::RequestParser reqParser_;
  detail::Cache reqCache_;
  detail::ResponseParser respParser_;
  detail::Cache respCache_;
  std::function<void(Yield)> confirm_;
  std::function<void(ConstBuffer<uint8_t>, Yield)> send_;
  std::function<size_t(MutableBuffer<uint8_t>, Yield)> recv_;
};

class HttpEgress : public Egress {
public:
  HttpEgress(detail::Socket&&);
  ~HttpEgress() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const&, Endpoint const&, Yield) override;

private:
  detail::Socket socket_;
  std::function<void(ConstBuffer<uint8_t>, Yield)> send_;
  std::function<size_t(MutableBuffer<uint8_t>, Yield)> recv_;
  detail::RequestParser reqParser_;
  detail::Cache reqCache_;
  detail::ResponseParser respParser_;
  detail::Cache respCache_;
};

} // namespace pichi::net

#endif // PICHI_NET_HTTP_HPP
