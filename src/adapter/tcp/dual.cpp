#include "pichi/common/config.hpp"
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/adapter/tcp/dual.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>
#include <utility>

namespace sys = boost::system;

namespace pichi::adapter::tcp {

template <stream::AsyncLayer NextLayer>
DualIngress<NextLayer>::DualIngress(vo::Ingress const& vo, NextLayer underlying)
  : vo_{vo}, underlying_{std::move(underlying)}, delegate_{std::nullopt}
{
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> DualIngress<NextLayer>::recv(MutableBuffer buf)
{
  assertTrue(delegate_.has_value());
  co_return co_await std::visit([=](auto&& ingress) { return ingress.recv(buf); }, *delegate_);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> DualIngress<NextLayer>::send(ConstBuffer buf)
{
  assertTrue(delegate_.has_value());
  co_await std::visit([=](auto&& ingress) { return ingress.send(buf); }, *delegate_);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> DualIngress<NextLayer>::close()
{
  assertTrue(delegate_.has_value());
  co_await std::visit([](auto&& ingress) { return ingress.close(); }, *delegate_);
}

template <stream::AsyncLayer NextLayer> Awaitable<Endpoint> DualIngress<NextLayer>::read_remote()
{
  co_await stream::accept(underlying_);

  auto buf = std::array<uint8_t, 1>{};
  co_await stream::read(underlying_, buf);

  if (buf[0] == 0x05_u8)
    delegate_.emplace(std::in_place_type<Socks5Ingress<NextLayer>>, vo_, std::move(underlying_));
  else
    delegate_.emplace(std::in_place_type<HttpIngress<NextLayer>>, vo_, std::move(underlying_));
  co_return co_await std::visit(
      [&](auto&& ingress) { return ingress.continue_read_remote(buf); },
      *delegate_
  );
}

template <stream::AsyncLayer NextLayer> Awaitable<void> DualIngress<NextLayer>::confirm()
{
  assertTrue(delegate_.has_value());
  co_await std::visit([](auto&& ingress) { return ingress.confirm(); }, *delegate_);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> DualIngress<NextLayer>::disconnect(sys::error_code const& ec)
{
  if (delegate_.has_value())
    co_await std::visit([&](auto&& ingress) { return ingress.disconnect(ec); }, *delegate_);
  co_return;
}

template class DualIngress<Socket>;
template class DualIngress<Tls>;

}  // namespace pichi::adapter::tcp
