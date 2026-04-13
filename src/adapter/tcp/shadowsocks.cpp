#include "pichi/common/config.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/stream/helpers.hpp>

namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::adapter::tcp {

template <typename Socket> auto create_stream(vo::Ingress const& vo, Socket s)
{
  assertTrue(vo.opt_.has_value());
  auto&& opt = std::get<vo::ShadowsocksOption>(*vo.opt_);
  return stream::Shadowsocks<Socket>{opt.method_, ConstBuffer{opt.password_}, std::move(s)};
}

template <typename Socket> auto create(vo::Egress const& vo, IOExecutor const& ex)
{
  assertTrue(vo.opt_.has_value());
  auto&& opt = std::get<vo::ShadowsocksOption>(*vo.opt_);
  return stream::Shadowsocks<Socket>{opt.method_, ConstBuffer{opt.password_}, *vo.server_, ex};
}

template <typename Socket>
Shadowsocks<Socket>::Shadowsocks(vo::Ingress const& vo, Socket s)
  : stream_{create_stream(vo, std::move(s))}
{
}

template <typename Socket>
Shadowsocks<Socket>::Shadowsocks(vo::Egress const& vo, IOExecutor const& ex)
  : stream_{create<Socket>(vo, ex)}
{
}

template <typename Socket> Awaitable<size_t> Shadowsocks<Socket>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(stream_, buf);
}

template <typename Socket> Awaitable<void> Shadowsocks<Socket>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <typename Socket> Awaitable<void> Shadowsocks<Socket>::close()
{
  co_await redirect(stream::close(stream_));
}

template <typename Socket> Awaitable<Endpoint> Shadowsocks<Socket>::read_remote()
{
  co_await stream::accept(stream_);
  co_return co_await parse_endpoint([this](auto buf) -> Awaitable<void> {
    co_await stream::read(stream_, buf);
  });
}

template <typename Socket> Awaitable<void> Shadowsocks<Socket>::confirm() { co_return; }

template <typename Socket> Awaitable<void> Shadowsocks<Socket>::connect(Endpoint const& peer)
{
  co_await stream::connect(stream_, peer);
}

template <typename Socket> Awaitable<void> Shadowsocks<Socket>::disconnect(sys::error_code const&)
{
  co_return;
}

template class Shadowsocks<asio::ip::tcp::socket>;

}  // namespace pichi::adapter::tcp
