#ifndef PICHI_ADAPTER_TCP_ADAPTER_HPP
#define PICHI_ADAPTER_TCP_ADAPTER_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/adapter/tcp/shadowsocks.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/stream/shadowsocks.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <variant>

namespace pichi::adapter::tcp {

template <CryptoMethod method, typename Socket = boost::asio::ip::tcp::socket>
using SSAdapter = Shadowsocks<stream::Shadowsocks<method, Socket>>;

using Ingress = std::variant<
    SSAdapter<CryptoMethod::AES_128_CTR>, SSAdapter<CryptoMethod::AES_192_CTR>,
    SSAdapter<CryptoMethod::AES_256_CTR>, SSAdapter<CryptoMethod::AES_128_CFB>,
    SSAdapter<CryptoMethod::AES_192_CFB>, SSAdapter<CryptoMethod::AES_256_CFB>,
    SSAdapter<CryptoMethod::CAMELLIA_128_CFB>, SSAdapter<CryptoMethod::CAMELLIA_192_CFB>,
    SSAdapter<CryptoMethod::CAMELLIA_256_CFB>, SSAdapter<CryptoMethod::CHACHA20>,
    SSAdapter<CryptoMethod::SALSA20>, SSAdapter<CryptoMethod::CHACHA20_IETF>,
    SSAdapter<CryptoMethod::AES_128_GCM>, SSAdapter<CryptoMethod::AES_192_GCM>,
    SSAdapter<CryptoMethod::AES_256_GCM>, SSAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>,
    SSAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>>;

using Egress = std::variant<
    SSAdapter<CryptoMethod::AES_128_CTR>, Socks5Egress<boost::asio::ip::tcp::socket>,
    SSAdapter<CryptoMethod::AES_192_CTR>, SSAdapter<CryptoMethod::AES_256_CTR>,
    SSAdapter<CryptoMethod::AES_128_CFB>, SSAdapter<CryptoMethod::AES_192_CFB>,
    SSAdapter<CryptoMethod::AES_256_CFB>, SSAdapter<CryptoMethod::CAMELLIA_128_CFB>,
    SSAdapter<CryptoMethod::CAMELLIA_192_CFB>, SSAdapter<CryptoMethod::CAMELLIA_256_CFB>,
    SSAdapter<CryptoMethod::CHACHA20>, SSAdapter<CryptoMethod::SALSA20>,
    SSAdapter<CryptoMethod::CHACHA20_IETF>, SSAdapter<CryptoMethod::AES_128_GCM>,
    SSAdapter<CryptoMethod::AES_192_GCM>, SSAdapter<CryptoMethod::AES_256_GCM>,
    SSAdapter<CryptoMethod::CHACHA20_IETF_POLY1305>,
    SSAdapter<CryptoMethod::XCHACHA20_IETF_POLY1305>>;

template <typename Socket> Ingress create_ingress(vo::Ingress const&, Socket);

extern Egress create_egress(vo::Egress const&, IOExecutor const&);

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_ADAPTER_HPP
