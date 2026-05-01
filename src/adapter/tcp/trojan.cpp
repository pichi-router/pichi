#include "pichi/common/config.hpp"
#include <algorithm>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <botan/hash.h>
#include <botan/hex.h>
#include <pichi/adapter/tcp/trojan.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/error.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/stream/helpers.hpp>
#include <ranges>
#include <vector>

namespace asio = boost::asio;
namespace ip   = asio::ip;
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

template <typename Stream> Awaitable<void> Cache::read_from(Stream& stream, size_t n)
{
  while (size() < n) {
    buf_.commit(co_await stream::read_some(stream, available_));
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

template <stream::AsyncSocket Socket>
TrojanIngress<Socket>::TrojanIngress(vo::Ingress const& vo, Socket underlying)
  : stream_{
    vo.websocket_.has_value()
    ? Stream{
      std::in_place_type<stream::Websocket<stream::Tls<Socket>>>,
      vo.websocket_->path_,
      vo.websocket_->host_.value_or(""),
      stream::tls_context(*vo.tls_),
      std::move(underlying)
    }
    : Stream{
      std::in_place_type<stream::Tls<Socket>>,
      stream::tls_context(*vo.tls_),
      std::move(underlying)
    }
  }, cred_{vo}, remote_{std::get<vo::TrojanOption>(*vo.opt_).remote_}
{
}

template <stream::AsyncSocket Socket>
Awaitable<size_t> TrojanIngress<Socket>::recv(MutableBuffer buf)
{
  if (cache_.size() == 0) co_return co_await stream::read_some(stream_, buf);

  co_return cache_.take(buf);
}

template <stream::AsyncSocket Socket> Awaitable<void> TrojanIngress<Socket>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <stream::AsyncSocket Socket> Awaitable<void> TrojanIngress<Socket>::close()
{
  co_await redirect(stream::close(stream_));
}

template <stream::AsyncSocket Socket> Awaitable<Endpoint> TrojanIngress<Socket>::read_remote()
{
  try {
    co_await stream::accept(stream_);

    co_await cache_.read_from(stream_);

    assertTrue(cache_.size() >= PWD_LEN + 2, PichiError::BAD_PROTO);
    assertTrue(cred_.authenticate(cache_.take(PWD_LEN)), PichiError::UNAUTHENTICATED);
    assertTrue(cache_.take(2) == "\r\n"sv, PichiError::BAD_PROTO);

    co_await cache_.read_from(stream_);
    assertTrue(cache_.take(1) == "\x01"sv, PichiError::BAD_PROTO);

    auto remote = co_await parse_endpoint([&](auto demand) -> Awaitable<void> {
      co_await cache_.read_from(stream_, demand.size());
      cache_.take(demand);
    });

    co_await cache_.read_from(stream_, 2);
    assertTrue(cache_.take(2) == "\r\n"sv, PichiError::BAD_PROTO);

    cache_.clear();
    co_return remote;
  }
  catch (...) {
    if (stream_.index() == 0) {
      cache_.rollback();
      co_return remote_;
    }
    std::rethrow_exception(std::current_exception());
  }
}

template <stream::AsyncSocket Socket> Awaitable<void> TrojanIngress<Socket>::confirm()
{
  co_return;
}

template <stream::AsyncSocket Socket>
Awaitable<void> TrojanIngress<Socket>::disconnect(sys::error_code const&)
{
  co_return;
}

template class TrojanIngress<ip::tcp::socket>;

template <stream::AsyncSocket Socket>
TrojanEgress<Socket>::TrojanEgress(vo::Egress const& vo, IOExecutor const& ex)
  : cred_{trojan::sha224(std::get<vo::TrojanEgressCredential>(*vo.credential_).credential_)},
    peer_{*vo.server_},
    stream_{
      vo.websocket_.has_value()
      ? Stream{
        std::in_place_type<stream::Websocket<stream::Tls<Socket>>>,
        vo.websocket_->path_,
        vo.websocket_->host_.value_or(""),
        vo.tls_->sni_,
        stream::tls_context(*vo.tls_, vo.server_->host_),
        ex
      }
      : Stream{
        std::in_place_type<stream::Tls<Socket>>,
        vo.tls_->sni_,
        stream::tls_context(*vo.tls_, vo.server_->host_),
        ex
      }
    }
{
}

template <stream::AsyncSocket Socket>
Awaitable<size_t> TrojanEgress<Socket>::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(stream_, buf);
}

template <stream::AsyncSocket Socket> Awaitable<void> TrojanEgress<Socket>::send(ConstBuffer buf)
{
  co_await stream::write(stream_, buf);
}

template <stream::AsyncSocket Socket> Awaitable<void> TrojanEgress<Socket>::close()
{
  co_await redirect(stream::close(stream_));
}

template <stream::AsyncSocket Socket>
Awaitable<void> TrojanEgress<Socket>::connect(Endpoint const& remote)
{
  co_await stream::connect(stream_, peer_);

  auto buf = std::array<uint8_t, 512>{};

  auto it = rngs::begin(buf);

  it = rngs::copy(cred_, it).out;
  it = rngs::copy("\r\n\x01"sv, it).out;
  it += serializeEndpoint(remote, {it, 259_sz});
  it = rngs::copy("\r\n"sv, it).out;

  co_await stream::write(
      stream_,
      {rngs::begin(buf), static_cast<size_t>(rngs::distance(rngs::begin(buf), it))}
  );
}

template class TrojanEgress<ip::tcp::socket>;

}  // namespace pichi::adapter::tcp
