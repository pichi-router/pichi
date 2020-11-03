#ifndef PICHI_NET_HTTP_HPP
#define PICHI_NET_HTTP_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>
#include <functional>
#include <limits>
#include <optional>
#include <pichi/common/adapter.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/endpoint.hpp>
#include <string_view>
#include <utility>

namespace pichi::net {

namespace detail {

using Cache = boost::beast::flat_buffer;
using Body = boost::beast::http::empty_body;

template <bool isRequest> using Parser = boost::beast::http::parser<isRequest, Body>;
using RequestParser = Parser<true>;
using ResponseParser = Parser<false>;
template <bool isRequest> using Header = boost::beast::http::header<isRequest>;
template <bool isRequest> using Message = boost::beast::http::message<isRequest, Body>;
using Request = Message<true>;
using Response = Message<false>;

template <typename R, typename... Args> R badInvoking(Args&&...)
{
  using namespace std;
  fail("Bad invocation"sv);
}

// FIXME hardcode HTTP header limit to 1M
inline uint32_t const HEADER_LIMIT = 1024ul * 1024ul;

}  // namespace detail

template <typename Stream> class HttpIngress : public Ingress {
private:
  using Authenticator = std::optional<std::function<bool(std::string const&, std::string const&)>>;

public:
  template <typename... Args>
  HttpIngress(Authenticator auth, Args&&... args)
    : stream_{std::forward<Args>(args)...},
      confirm_{detail::badInvoking<void, Yield>},
      send_{detail::badInvoking<void, ConstBuffer<uint8_t>, Yield>},
      recv_{detail::badInvoking<size_t, MutableBuffer<uint8_t>, Yield>},
      auth_{std::move(auth)}
  {
    reqParser_.header_limit(detail::HEADER_LIMIT);
    reqParser_.body_limit(std::numeric_limits<uint64_t>::max());
    respParser_.header_limit(detail::HEADER_LIMIT);
    respParser_.body_limit(std::numeric_limits<uint64_t>::max());
  }

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;

  void close(Yield) override;

  bool readable() const override;
  bool writable() const override;
  void confirm(Yield yield) override;
  void disconnect(std::exception_ptr, Yield) override;
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
  Authenticator auth_;
};

template <typename Stream> class HttpEgress : public Egress {
private:
  using Credential = std::pair<std::string, std::string>;

public:
  template <typename... Args>
  HttpEgress(std::optional<Credential> credential, Args&&... args)
    : stream_{std::forward<Args>(args)...},
      send_(detail::badInvoking<void, ConstBuffer<uint8_t>, Yield>),
      recv_(detail::badInvoking<size_t, MutableBuffer<uint8_t>, Yield>),
      credential_{std::move(credential)}
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
  void connect(Endpoint const&, ResolveResults, Yield) override;

private:
  Stream stream_;
  std::function<void(ConstBuffer<uint8_t>, Yield)> send_;
  std::function<size_t(MutableBuffer<uint8_t>, Yield)> recv_;
  detail::RequestParser reqParser_;
  detail::Cache reqCache_;
  detail::ResponseParser respParser_;
  detail::Cache respCache_;
  std::optional<Credential> credential_;
};

}  // namespace pichi::net

#endif  // PICHI_NET_HTTP_HPP
