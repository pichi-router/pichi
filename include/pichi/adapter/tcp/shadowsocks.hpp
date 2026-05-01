#ifndef PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
#define PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP

#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/shadowsocks.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>

namespace pichi::adapter::tcp {

namespace detail {

template <stream::AsyncSocket Socket> auto create_stream(vo::Ingress const& vo, Socket s)
{
  assertTrue(vo.opt_.has_value());
  auto&& opt = std::get<vo::ShadowsocksOption>(*vo.opt_);
  return stream::Shadowsocks<Socket>{opt.method_, ConstBuffer{opt.password_}, std::move(s)};
}

template <stream::AsyncSocket Socket> auto create_stream(vo::Egress const& vo, IOExecutor const& ex)
{
  assertTrue(vo.opt_.has_value());
  auto&& opt = std::get<vo::ShadowsocksOption>(*vo.opt_);
  return stream::Shadowsocks<Socket>{opt.method_, ConstBuffer{opt.password_}, *vo.server_, ex};
}

}  // namespace detail

template <stream::AsyncSocket Socket> class Shadowsocks {
public:
  Shadowsocks(vo::Ingress const& vo, Socket s) : stream_{detail::create_stream(vo, std::move(s))} {}

  Shadowsocks(vo::Egress const& vo, IOExecutor const& ex)
    : stream_{detail::create_stream<Socket>(vo, ex)}
  {
  }

  Awaitable<size_t> recv(MutableBuffer b) { co_return co_await stream::read_some(stream_, b); }
  Awaitable<void>   send(ConstBuffer b) { co_await stream::write(stream_, b); }

  Awaitable<void> close() { co_await redirect(stream::close(stream_)); }

  Awaitable<Endpoint> read_remote()
  {
    co_await stream::accept(stream_);
    co_return co_await parse_endpoint([this](auto buf) -> Awaitable<void> {
      co_await stream::read(stream_, buf);
    });
  }

  Awaitable<void> confirm() { co_return; }

  Awaitable<void> connect(Endpoint const& peer) { co_await stream::connect(stream_, peer); }

  Awaitable<void> disconnect(boost::system::error_code const&) { co_return; }

private:
  stream::Shadowsocks<Socket> stream_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_SHADOWSOCKS_HPP
