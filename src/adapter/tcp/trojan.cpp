#include "pichi/common/config.hpp"
#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <pichi/adapter/tcp/adapter.hpp>
#include <pichi/adapter/tcp/trojan.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>
#include <pichi/stream/test.hpp>
#include <ranges>
#include <vector>

namespace rngs = std::ranges;
namespace sys  = boost::system;
namespace view = std::views;

using namespace std::literals;

namespace pichi::adapter::tcp {

static auto const PWD_LEN = 56_sz;

namespace trojan {

static std::string sha224(std::string_view pwd)
{
  auto sha224 = Botan::HashFunction::create_or_throw("SHA-224");
  sha224->update(pwd);
  return Botan::hex_encode(sha224->final(), false);
}

template <typename Container> Container create_pwds(vo::TrojanIngressCredential const& vo)
{
  auto v = vo.credential_ | view::transform([](auto&& cred) { return sha224(cred); });
  return Container{rngs::cbegin(v), rngs::cend(v)};
}

size_t IngressCredentials::StringHash::operator()(char const* key) const
{
  return std::invoke(hash_type{}, key);
}

size_t IngressCredentials::StringHash::operator()(std::string_view key) const
{
  return std::invoke(hash_type{}, key);
}
size_t IngressCredentials::StringHash::operator()(std::string const& key) const
{
  return std::invoke(hash_type{}, key);
}

IngressCredentials::IngressCredentials(vo::Ingress const& vo)
  : passwords_{create_pwds<Passwords>(std::get<vo::TrojanIngressCredential>(*vo.credential_))}
{
}

bool IngressCredentials::authenticate(std::string_view str) const
{
  return passwords_.contains(str);
}

Cache::Cache() : available_{buf_.prepare(512)} {}

template <stream::AsyncLayer Layer> Awaitable<void> Cache::read_from(Layer& underlying, size_t n)
{
  while (size() < n) {
    buf_.commit(co_await stream::read_some(underlying, available_));
    available_ += buf_.size();
  }
}

size_t Cache::size() const { return buf_.size() - taken_; }

std::string_view Cache::take(size_t n)
{
  assertTrue(n <= size());
  auto p = static_cast<char const*>(buf_.cdata().data()) + taken_;
  taken_ += n;
  return {p, n};
}

size_t Cache::take(MutableBuffer dst)
{
  auto copied = std::min(size(), dst.size());
  std::copy_n(static_cast<char const*>(buf_.cdata().data()) + taken_, copied, std::begin(dst));
  if (taken_ == 0)
    buf_.consume(copied);
  else
    taken_ += copied;
  return copied;
}

void Cache::clear()
{
  buf_.consume(taken_);
  taken_ = 0;
}

void Cache::rollback() { taken_ = 0; }

}  // namespace trojan

template <stream::AsyncLayer NextLayer>
TrojanIngress<NextLayer>::TrojanIngress(vo::Ingress const& vo, NextLayer underlying)
  : underlying_{std::move(underlying)},
    cred_{vo},
    remote_{std::get<vo::TrojanOption>(*vo.opt_).remote_}
{
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> TrojanIngress<NextLayer>::recv(MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(underlying_, buf);

  co_return cache_.take(buf);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> TrojanIngress<NextLayer>::send(ConstBuffer buf)
{
  co_await stream::write(underlying_, buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> TrojanIngress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer> Awaitable<Endpoint> TrojanIngress<NextLayer>::read_remote()
{
  try {
    co_await stream::accept(underlying_);

    co_await cache_.read_from(underlying_);

    assertTrue(cache_.size() >= PWD_LEN + 2, PichiError::BAD_PROTO);
    assertTrue(cred_.authenticate(cache_.take(PWD_LEN)), PichiError::UNAUTHENTICATED);
    assertTrue(cache_.take(2) == "\r\n"sv, PichiError::BAD_PROTO);

    co_await cache_.read_from(underlying_);
    assertTrue(cache_.take(1) == "\x01"sv, PichiError::BAD_PROTO);

    auto remote = co_await parse_endpoint([&](auto demand) -> Awaitable<void> {
      co_await cache_.read_from(underlying_, demand.size());
      cache_.take(demand);
    });

    co_await cache_.read_from(underlying_, 2);
    assertTrue(cache_.take(2) == "\r\n"sv, PichiError::BAD_PROTO);

    cache_.clear();
    co_return remote;
  }
  catch (...) {
    if constexpr (std::same_as<NextLayer, Tls> || std::same_as<NextLayer, unit_test::TestSocket>) {
      cache_.rollback();
      co_return remote_;
    }
    std::rethrow_exception(std::current_exception());
  }
}

template <stream::AsyncLayer NextLayer> Awaitable<void> TrojanIngress<NextLayer>::confirm()
{
  co_return;
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> TrojanIngress<NextLayer>::disconnect(sys::error_code const&)
{
  co_return;
}

template class TrojanIngress<Tls>;
template class TrojanIngress<Websocket>;
template class TrojanIngress<unit_test::TestSocket>;

template <stream::AsyncLayer NextLayer>
TrojanEgress<NextLayer>::TrojanEgress(vo::Egress const& vo, NextLayer underlying)
  : cred_{trojan::sha224(std::get<vo::TrojanEgressCredential>(*vo.credential_).credential_)},
    peer_{*vo.server_},
    underlying_{std::move(underlying)}
{
}

template <stream::AsyncLayer NextLayer>
Awaitable<size_t> TrojanEgress<NextLayer>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(underlying_, buf);
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> TrojanEgress<NextLayer>::send(ConstBuffer buf)
{
  co_await stream::write(underlying_, buf);
}

template <stream::AsyncLayer NextLayer> Awaitable<void> TrojanEgress<NextLayer>::close()
{
  co_await redirect(stream::close(underlying_));
}

template <stream::AsyncLayer NextLayer>
Awaitable<void> TrojanEgress<NextLayer>::connect(Endpoint const& remote)
{
  co_await stream::connect(underlying_, peer_);

  auto buf = std::array<uint8_t, 512>{};

  auto it = rngs::begin(buf);

  it = rngs::copy(cred_, it).out;
  it = rngs::copy("\r\n\x01"sv, it).out;
  it += serializeEndpoint(remote, {it, 259_sz});
  it = rngs::copy("\r\n"sv, it).out;

  co_await stream::write(
      underlying_,
      {rngs::begin(buf), static_cast<size_t>(rngs::distance(rngs::begin(buf), it))}
  );
}

template class TrojanEgress<Tls>;
template class TrojanEgress<Websocket>;
template class TrojanEgress<unit_test::TestSocket>;

}  // namespace pichi::adapter::tcp
