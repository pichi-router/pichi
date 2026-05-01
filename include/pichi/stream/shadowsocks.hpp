#ifndef PICHI_STREAM_SHADOWSOCKS_HPP
#define PICHI_STREAM_SHADOWSOCKS_HPP

#include <array>
#include <boost/asio/async_result.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <botan/cipher_mode.h>
#include <memory>
#include <mutex>
#include <pichi/common/asserts.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/stream/helpers.hpp>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

namespace pichi::stream {

namespace detail {

class SaltSentry {
private:
  using Salts  = std::unordered_set<std::string>;
  using Strand = boost::asio::strand<IOExecutor>;
  using Timer  = boost::asio::system_timer;

  Awaitable<void> run();

public:
  SaltSentry(IOExecutor const&);

  Awaitable<bool> contains(ConstBuffer);
  Awaitable<void> reset();

  void start();
  void cancel();

private:
  Strand strand_;
  Timer  timer_;
  Salts  expiring_ = {};
  Salts  active_   = {};
};

class SaltSentryService
  : public boost::asio::detail::execution_context_service_base<SaltSentryService> {
public:
  explicit SaltSentryService(boost::asio::execution_context&);

  std::shared_ptr<SaltSentry> sentry(IOExecutor const&);

private:
  void shutdown() noexcept override;

  std::once_flag              flag_   = {};
  std::shared_ptr<SaltSentry> sentry_ = nullptr;
};

extern std::shared_ptr<SaltSentry> get_sentry(IOExecutor const&);

template <ExecutionContext Context> auto get_sentry(Context& ctx)
{
  return get_sentry(ctx.get_executor());
}

class Cryptor {
public:
  Cryptor(CryptoMethod, Botan::Cipher_Dir);

  void set_psk(ConstBuffer, ConstBuffer);

  size_t process(ConstBuffer, MutableBuffer);

private:
  std::unique_ptr<Botan::Cipher_Mode> cryptor_;

  std::vector<uint8_t> nonce_;
};

class Encryptor {
public:
  Encryptor(CryptoMethod, ConstBuffer);

  ConstBuffer salt() const;

  size_t process(ConstBuffer, MutableBuffer);

private:
  Cryptor cryptor_;

  std::vector<uint8_t> salt_;
};

class Decryptor {
public:
  Decryptor(CryptoMethod);

  MutableBuffer salt();

  void   set_psk(ConstBuffer);
  size_t process(ConstBuffer, MutableBuffer);

private:
  Cryptor cryptor_;

  std::vector<uint8_t> salt_;
};

class Cache {
public:
  Cache() = default;

  bool empty() const;

  size_t copy(MutableBuffer);

  MutableBuffer prepare(MutableBuffer, size_t);

private:
  boost::beast::flat_buffer data_ = {};
};

}  // namespace detail

template <AsyncSocket Socket> class Shadowsocks {
private:
  static constexpr size_t FRAME_SIZE = 0x3fff;
  static constexpr size_t TAG_SIZE   = 16;

  using Sentry    = std::shared_ptr<detail::SaltSentry>;
  using ErrorCode = boost::system::error_code;
  using Encryptor = detail::Encryptor;
  using Decryptor = detail::Decryptor;

  using PlainBuffer  = std::array<uint8_t, FRAME_SIZE>;
  using CipherBuffer = std::array<uint8_t, FRAME_SIZE + 2 + 2 * TAG_SIZE>;
  using LengthBuffer = std::array<uint8_t, 2>;

  using Data  = std::vector<uint8_t>;
  using Cache = detail::Cache;

  Awaitable<void> read_salt()
  {
    auto salt = decryptor_.salt();
    co_await boost::asio::async_read(
        socket_,
        boost::asio::buffer(std::ranges::data(salt), std::ranges::size(salt)),
        boost::asio::use_awaitable
    );
    if (sentry_) assertFalse(co_await sentry_->contains(salt), PichiError::BAD_PROTO);
    decryptor_.set_psk(pw_);
    pw_.clear();
  }

  Awaitable<void> write_salt()
  {
    co_await boost::asio::async_write(
        socket_,
        boost::asio::buffer(encryptor_.salt()),
        boost::asio::use_awaitable
    );
  }

  Awaitable<void> read_block(MutableBuffer block)
  {
    auto cipher = CipherBuffer{};
    auto len    = std::ranges::size(block) + TAG_SIZE;

    co_await boost::asio::async_read(
        socket_,
        boost::asio::buffer(cipher, len),
        boost::asio::use_awaitable
    );
    decryptor_.process({cipher, len}, block);
  }

  Awaitable<void> do_connect(Endpoint const& peer)
  {
    co_await connect(socket_, proxy_);

    auto plain = PlainBuffer{};
    auto len   = serializeEndpoint(peer, plain);
    co_await do_write({plain, len});
  }

  Awaitable<size_t> do_read(MutableBuffer plain)
  {
    if (!std::ranges::empty(pw_)) co_await read_salt();

    if (!cache_.empty()) co_return cache_.copy(plain);

    auto lb = LengthBuffer{};
    co_await read_block(lb);

    auto len = ntoh<uint16_t>(lb);
    assertTrue(len <= FRAME_SIZE, PichiError::BAD_PROTO);

    co_await read_block(cache_.prepare(plain, len));
    co_return cache_.empty() ? len : cache_.copy(plain);
  }

  Awaitable<size_t> do_write(ConstBuffer plain)
  {
    if (!std::ranges::empty(encryptor_.salt())) co_await write_salt();

    auto data = CipherBuffer{};
    auto ret  = size_t{0};
    while (std::ranges::size(plain) > 0) {
      auto cipher = MutableBuffer{data};

      auto lb = LengthBuffer{};
      auto pl = std::min(std::ranges::size(plain), FRAME_SIZE);
      hton(static_cast<uint16_t>(pl), lb);
      auto cl = encryptor_.process(lb, cipher);
      cl += encryptor_.process({plain, pl}, cipher + cl);
      co_await boost::asio::async_write(
          socket_,
          boost::asio::buffer(data, cl),
          boost::asio::use_awaitable
      );
      plain += pl;
      ret += cl;
    }
    co_return ret;
  }

public:
  using endpoint_type   = Endpoint;
  using executor_type   = typename Socket::executor_type;
  using next_layer_type = Socket;

  Shadowsocks(CryptoMethod method, ConstBuffer pw, Socket socket)
    : pw_{std::ranges::begin(pw), std::ranges::end(pw)},
      sentry_{detail::get_sentry(socket.get_executor())},
      socket_{std::move(socket)},
      encryptor_{method, pw_},
      decryptor_{method}
  {
  }

  Shadowsocks(CryptoMethod method, ConstBuffer pw, Endpoint const& proxy, IOExecutor const& ex)
    : pw_{std::ranges::begin(pw), std::ranges::end(pw)},
      sentry_{nullptr},
      socket_{ex},
      encryptor_{method, pw_},
      decryptor_{method},
      proxy_{proxy}
  {
  }

  executor_type get_executor() { return socket_.get_executor(); }

  next_layer_type&       next_layer() { return socket_; }
  next_layer_type const& next_layer() const { return socket_; }

  bool is_open() const { return socket_.is_open(); }

  template <typename ShutdownToken> auto async_shutdown(ShutdownToken&& token)
  {
    return stream::async_initiate<void(ErrorCode)>(std::forward<ShutdownToken>(token), ErrorCode{});
  }

  template <typename AcceptToken> auto async_accept(AcceptToken&& token)
  {
    return stream::async_initiate<void(ErrorCode)>(
        get_executor(),
        std::forward<AcceptToken>(token),
        [this]() { return read_salt(); }
    );
  }

  template <typename MutableBufferSequence, typename ReadToken>
  auto async_read_some(MutableBufferSequence const& b, ReadToken&& token)
  {
    return stream::async_initiate<void(ErrorCode, size_t)>(
        get_executor(),
        std::forward<ReadToken>(token),
        [this](auto&& b) { return do_read(b); },
        b
    );
  }

  template <typename ConstBufferSequence, typename WriteToken>
  auto async_write_some(ConstBufferSequence const& b, WriteToken&& token)
  {
    return stream::async_initiate<void(ErrorCode, size_t)>(
        get_executor(),
        std::forward<WriteToken>(token),
        [this](auto b) { return do_write(b); },
        b
    );
  }

  template <typename ConnectToken> auto async_connect(Endpoint const& peer, ConnectToken&& token)
  {
    return stream::async_initiate<void(ErrorCode)>(
        get_executor(),
        std::forward<ConnectToken>(token),
        [this, &peer]() { return do_connect(peer); }
    );
  }

private:
  Data  pw_;
  Cache cache_ = {};

  Sentry    sentry_;
  Socket    socket_;
  Encryptor encryptor_;
  Decryptor decryptor_;

  Endpoint proxy_ = {};
};

}  // namespace pichi::stream

#endif  // PICHI_STREAM_SHADOWSOCKS_HPP
