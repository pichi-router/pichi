#ifndef PICHI_NET_HTTP_HPP
#define PICHI_NET_HTTP_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <limits>
#include <pichi/asserts.hpp>
#include <pichi/buffer.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pichi::net {

namespace detail {

using Cache = boost::beast::flat_buffer;
using Body = boost::beast::http::empty_body;

template <bool isRequest> using Parser = boost::beast::http::parser<isRequest, Body>;
using RequestParser = Parser<true>;
using ResponseParser = Parser<false>;

template <typename R, typename... Args> R badInvoking(Args&&...)
{
  using namespace std;
  fail("Bad invocation"sv);
}

// FIXME hardcode HTTP header limit to 1M
inline uint32_t const HEADER_LIMIT = 1024ul * 1024ul;

} // namespace detail

template <typename Stream> class HttpIngress : public Ingress {
public:
  template <typename... Args>
  HttpIngress(Args&&... args)
    : stream_{std::forward<Args>(args)...}, confirm_{detail::badInvoking<void, Yield>},
      send_(detail::badInvoking<void, ConstBuffer<uint8_t>, Yield>),
      recv_(detail::badInvoking<size_t, MutableBuffer<uint8_t>, Yield>)
  {
    reqParser_.header_limit(detail::HEADER_LIMIT);
    reqParser_.body_limit(std::numeric_limits<uint64_t>::max());
    respParser_.header_limit(detail::HEADER_LIMIT);
    respParser_.body_limit(std::numeric_limits<uint64_t>::max());
  }

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;

  void send(ConstBuffer<uint8_t>, Yield) override;

  void close(Yield) override;

  bool readable() const override;

  bool writable() const override;

  void confirm(Yield yield) override;

  void disconnect(Yield) override;

  Endpoint readRemote(Yield) override;

private:
  Stream stream_;
  detail::RequestParser reqParser_;
  detail::Cache reqCache_;
  detail::ResponseParser respParser_;
  detail::Cache respCache_;
  std::function<void(Yield)> confirm_;
  std::function<void(ConstBuffer<uint8_t>, Yield)> send_;
  std::function<size_t(MutableBuffer<uint8_t>, Yield)> recv_;
};

template <typename Stream> class HttpEgress : public Egress {
public:
  template <typename... Args>
  HttpEgress(Args&&... args)
    : origin_{std::forward<Args>(args)...}, backup_{std::forward<Args>(args)...},
      stream_{std::addressof(origin_)},
      send_(detail::badInvoking<void, ConstBuffer<uint8_t>, Yield>),
      recv_(detail::badInvoking<size_t, MutableBuffer<uint8_t>, Yield>)
  {
    reqParser_.header_limit(detail::HEADER_LIMIT);
    reqParser_.body_limit(std::numeric_limits<uint64_t>::max());
    respParser_.header_limit(detail::HEADER_LIMIT);
    respParser_.body_limit(std::numeric_limits<uint64_t>::max());
  }

  ~HttpEgress() override = default;

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const&, Endpoint const&, Yield) override;

private:
  Stream origin_;
  Stream backup_;
  std::add_pointer_t<Stream> stream_;
  std::function<void(ConstBuffer<uint8_t>, Yield)> send_;
  std::function<size_t(MutableBuffer<uint8_t>, Yield)> recv_;
  detail::RequestParser reqParser_;
  detail::Cache reqCache_;
  detail::ResponseParser respParser_;
  detail::Cache respCache_;
};

} // namespace pichi::net

#endif // PICHI_NET_HTTP_HPP
