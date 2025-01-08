#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/helpers.hpp>

#define ADAPTER_FUNC(Ret)                                                                          \
  template <CryptoMethod method, typename Socket> Ret Shadowsocks<method, Socket>
#define DECL_ADAPTER(Method) template class Shadowsocks<CryptoMethod::Method, asio::ip::tcp::socket>

namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::adapter::tcp {

template <CryptoMethod method, typename Socket>
Shadowsocks<method, Socket>::Shadowsocks(ConstBuffer psk, Socket s) : stream_{psk, std::move(s)}
{
}

template <CryptoMethod method, typename Socket>
Shadowsocks<method, Socket>::Shadowsocks(ConstBuffer psk, IOExecutor const& ex) : stream_{psk, ex}
{
}

ADAPTER_FUNC(Awaitable<size_t>)::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(stream_, buf);
}

ADAPTER_FUNC(Awaitable<void>)::send(ConstBuffer buf) { co_await stream::write(stream_, buf); }

ADAPTER_FUNC(Awaitable<void>)::close() { co_await redirect(stream::close(stream_)); }

ADAPTER_FUNC(Awaitable<Endpoint>)::read_remote()
{
  co_await stream::accept(stream_);
  co_return co_await parse_endpoint([this](auto buf) -> Awaitable<void> {
    co_await stream::read(stream_, buf);
  });
}

ADAPTER_FUNC(Awaitable<void>)::confirm() { co_return; }

ADAPTER_FUNC(Awaitable<void>)::connect(Endpoint const& peer)
{
  co_await stream::connect(stream_, peer);
}

ADAPTER_FUNC(Awaitable<void>)::disconnect(sys::error_code const&) { co_return; }

DECL_ADAPTER(AES_128_CTR);
DECL_ADAPTER(AES_192_CTR);
DECL_ADAPTER(AES_256_CTR);
DECL_ADAPTER(AES_128_CFB);
DECL_ADAPTER(AES_192_CFB);
DECL_ADAPTER(AES_256_CFB);
DECL_ADAPTER(CAMELLIA_128_CFB);
DECL_ADAPTER(CAMELLIA_192_CFB);
DECL_ADAPTER(CAMELLIA_256_CFB);
DECL_ADAPTER(CHACHA20);
DECL_ADAPTER(SALSA20);
DECL_ADAPTER(CHACHA20_IETF);
DECL_ADAPTER(AES_128_GCM);
DECL_ADAPTER(AES_192_GCM);
DECL_ADAPTER(AES_256_GCM);
DECL_ADAPTER(CHACHA20_IETF_POLY1305);
DECL_ADAPTER(XCHACHA20_IETF_POLY1305);

}  // namespace pichi::adapter::tcp
