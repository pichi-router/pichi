#ifndef PICHI_ADAPTER_TCP_DUAL_HPP
#define PICHI_ADAPTER_TCP_DUAL_HPP

#include <boost/system/error_code.hpp>
#include <optional>
#include <pichi/adapter/tcp/http.hpp>
#include <pichi/adapter/tcp/socks5.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/concepts.hpp>
#include <variant>

namespace pichi::adapter::tcp {

template <stream::AsyncLayer NextLayer> class DualIngress {
private:
  using Delegate = std::optional<std::variant<HttpIngress<NextLayer>, Socks5Ingress<NextLayer>>>;

public:
  explicit DualIngress(vo::Ingress const&, NextLayer);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  vo::Ingress vo_;
  NextLayer   underlying_;
  Delegate    delegate_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_DUAL_HPP
