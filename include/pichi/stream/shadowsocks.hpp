#ifndef PICHI_STREAM_SHADOWSOCKS_HPP
#define PICHI_STREAM_SHADOWSOCKS_HPP

#include <array>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/constants.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <pichi/common/error.hpp>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/method.hpp>
#include <pichi/crypto/stream.hpp>
#include <pichi/net/helper.hpp>
#include <pichi/stream/completer.hpp>
#include <pichi/stream/helpers.hpp>
#include <utility>

namespace pichi::stream {

template <CryptoMethod method, typename Socket> class Shadowsocks {
private:
  using Decryptor = crypto::Decryptor<method>;
  using Encryptor = crypto::Encryptor<method>;
  using ErrorCode = boost::system::error_code;

  using Cache        = boost::beast::basic_flat_buffer<std::allocator<uint8_t>>;
  using PlainBuffer  = std::array<uint8_t, MAX_FRAME_SIZE>;
  using CipherBuffer = std::array<uint8_t, MAX_FRAME_SIZE + 2 + 2 * crypto::TAG_SIZE<method>>;
  using IVBuffer     = std::array<uint8_t, crypto::IV_SIZE<method>>;
  using LenBuffer    = std::array<uint8_t, 2>;

  size_t copy_to(MutableBuffer dst)
  {
    auto copied = boost::asio::buffer_copy(boost::asio::buffer(dst), cache_.data(), cache_.size());
    cache_.consume(copied);
    return copied;
  }

  Awaitable<void> read_iv()
  {
    if (ivRecv_) co_return;

    auto iv = IVBuffer{};
    co_await boost::asio::async_read(socket_, boost::asio::buffer(iv), boost::asio::use_awaitable);
    decryptor_.setIv(iv);
    ivRecv_ = true;
  }

  Awaitable<void> write_iv()
  {
    if (ivSent_) co_return;

    co_await boost::asio::async_write(
        socket_,
        boost::asio::buffer(encryptor_.getIv()),
        boost::asio::use_awaitable
    );
    ivSent_ = true;
  }

  Awaitable<size_t> do_stream_read(MutableBuffer plain)
  {
    auto ex = get_executor();
    co_await switch_to(ex);

    auto cipher = CipherBuffer{};
    auto len =
        co_await socket_.async_read_some(boost::asio::buffer(cipher, plain.size()), await_to(ex));
    co_return decryptor_.decrypt({cipher, len}, plain);
  }

  MutableBuffer prepare(MutableBuffer provided, size_t n)
  {
    if (n <= provided.size()) return {provided, n};
    auto buf = cache_.prepare(n);
    cache_.commit(n);
    return buf;
  }

  Awaitable<void> read_block(MutableBuffer block)
  {
    auto cipher = CipherBuffer{};
    auto len    = block.size() + crypto::TAG_SIZE<method>;

    co_await boost::asio::async_read(
        socket_,
        boost::asio::buffer(cipher, len),
        await_to(get_executor())
    );
    decryptor_.decrypt({cipher, len}, block);
  }

  Awaitable<void> do_connect(Endpoint const& peer)
  {
    co_await connect(socket_, proxy_);
    auto plain = PlainBuffer{};
    auto len   = serializeEndpoint(peer, plain);
    co_await do_write({plain, len});
  }

  Awaitable<size_t> do_aead_read(MutableBuffer plain)
  {
    auto ex = get_executor();
    co_await switch_to(ex);

    if (cache_.size() > 0) co_return copy_to(plain);

    auto lb = LenBuffer{};
    co_await read_block(lb);

    auto len = ntoh<uint16_t>(lb);
    assertTrue(len <= MAX_FRAME_SIZE, PichiError::BAD_PROTO);

    co_await read_block(prepare(plain, len));

    co_return cache_.size() == 0 ? len : copy_to(plain);
  }

  Awaitable<size_t> do_read(MutableBuffer plain)
  {
    co_await read_iv();

    if constexpr (crypto::detail::isStream<method>()) {
      co_return co_await do_stream_read(plain);
    }
    else {
      co_return co_await do_aead_read(plain);
    }
  }

  Awaitable<size_t> do_stream_write(ConstBuffer plain)
  {
    auto ex = get_executor();
    co_await switch_to(ex);

    auto cipher = CipherBuffer{};
    auto ret    = plain.size();
    while (plain.size() > 0) {
      auto len = std::min(plain.size(), cipher.size());
      encryptor_.encrypt({plain, len}, cipher);
      co_await boost::asio::async_write(socket_, boost::asio::buffer(cipher, len), await_to(ex));
      plain += len;
    }
    co_return ret;
  }

  Awaitable<size_t> do_aead_write(ConstBuffer plain)
  {
    auto ex = get_executor();
    co_await switch_to(ex);

    auto data = CipherBuffer{};
    auto ret  = 0_sz;
    while (plain.size() > 0) {
      auto cipher = MutableBuffer{data};

      auto lb = LenBuffer{};
      auto pl = std::min(plain.size(), MAX_FRAME_SIZE);
      hton(static_cast<uint16_t>(pl), lb);
      auto cl = encryptor_.encrypt(lb, cipher);
      cl += encryptor_.encrypt({plain, pl}, cipher + cl);
      co_await boost::asio::async_write(socket_, boost::asio::buffer(data, cl), await_to(ex));
      plain += pl;
      ret += cl;
    }
    co_return ret;
  }

  Awaitable<size_t> do_write(ConstBuffer plain)
  {
    co_await write_iv();

    if constexpr (crypto::detail::isStream<method>()) {
      co_return co_await do_stream_write(plain);
    }
    else {
      co_return co_await do_aead_write(plain);
    }
  }

public:
  using executor_type   = typename Socket::executor_type;
  using next_layer_type = Socket;

  Shadowsocks(ConstBuffer psk, Socket socket)
    : socket_{std::move(socket)}, encryptor_{psk}, decryptor_{psk}
  {
  }

  Shadowsocks(ConstBuffer psk, Endpoint const& proxy, IOExecutor const& ex)
    : socket_{ex}, encryptor_{psk}, decryptor_{psk}, proxy_{proxy}
  {
  }

  executor_type const& get_executor() { return socket_.get_executor(); }

  next_layer_type&       next_layer() { return socket_; }
  next_layer_type const& next_layer() const { return socket_; }

  bool is_open() const { return socket_.is_open(); }

  template <typename ShutdownToken> auto async_shutdown(ShutdownToken&& token)
  {
    return async_initiate<void(ErrorCode)>(std::forward<ShutdownToken>(token), ErrorCode{});
  }

  template <typename AcceptToken> auto async_accept(AcceptToken&& token)
  {
    return async_initiate<void(ErrorCode)>(
        get_executor(),
        std::forward<AcceptToken>(token),
        [this]() { return read_iv(); }
    );
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read_some(MutableBufferSequence const& b, ReadToken&& token)
  {
    return async_initiate<void(ErrorCode, size_t)>(
        get_executor(),
        std::forward<ReadToken>(token),
        [this](auto&& b) { return do_read(b); },
        b
    );
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write_some(ConstBufferSequence const& b, WriteToken&& token)
  {
    return async_initiate<void(ErrorCode, size_t)>(
        get_executor(),
        std::forward<WriteToken>(token),
        [this](auto b) { return do_write(b); },
        b
    );
  }

  template <typename ConnectToken> auto async_connect(Endpoint const& peer, ConnectToken&& token)
  {
    return async_initiate<void(ErrorCode)>(
        get_executor(),
        std::forward<ConnectToken>(token),
        [this, &peer]() { return do_connect(peer); }
    );
  }

private:
  Socket    socket_;
  Encryptor encryptor_;
  Decryptor decryptor_;
  bool      ivSent_ = false;
  bool      ivRecv_ = false;
  Cache     cache_  = {};
  Endpoint  proxy_  = {};
};

}  // namespace pichi::stream

#endif  // PICHI_STREAM_SHADOWSOCKS_HPP
