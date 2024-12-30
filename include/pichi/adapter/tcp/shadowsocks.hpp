#ifndef PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
#define PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP

#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>

namespace pichi::adapter::tcp {

template <typename Stream> class Shadowsocks {
public:
  template <typename... Args>
  requires(std::constructible_from<Stream, Args...>)
  explicit Shadowsocks(Args&&... args) : stream_{std::forward<Args>(args)...}
  {
  }

  Shadowsocks(Shadowsocks&&) noexcept = default;

  ~Shadowsocks() = default;

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);

  Awaitable<void> close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();

  Awaitable<void> connect(Endpoint const&);
  Awaitable<void> disconnect(boost::system::error_code const&);

private:
  Stream stream_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
