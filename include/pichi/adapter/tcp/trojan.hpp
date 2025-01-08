#ifndef PICHI_ADAPTER_TCP_TROJAN_HPP
#define PICHI_ADAPTER_TCP_TROJAN_HPP

#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/system/error_code.hpp>
#include <concepts>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/stream/websocket.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <string>
#include <string_view>
#include <unordered_set>

namespace pichi::adapter::tcp {

namespace trojan {

class IngressCredentials {
private:
  struct StringHash {
    using hash_type      = std::hash<std::string_view>;
    using is_transparent = void;

    size_t operator()(char const*) const;
    size_t operator()(std::string_view) const;
    size_t operator()(std::string const&) const;
  };

  using Passwords = std::unordered_set<std::string, StringHash, std::equal_to<>>;

public:
  explicit IngressCredentials(vo::Ingress const&);

  bool authenticate(std::string_view) const;

private:
  Passwords passwords_;
};

class Cache {
private:
  using Buffer = boost::beast::flat_static_buffer<512>;

public:
  Cache();

  size_t size() const;

  template <typename Stream> Awaitable<void> read_from(Stream&, size_t = 1);

  std::string_view take(size_t);

  size_t take(MutableBuffer);

  void clear();
  void rollback();

private:
  Buffer        buf_   = {};
  size_t        taken_ = 0;
  MutableBuffer available_;
};

}  // namespace trojan

template <typename Stream> class TrojanIngress {
private:
  using Credential = trojan::IngressCredentials;
  using Cache      = trojan::Cache;

  template <typename... Args>
  requires(std::constructible_from<Stream, Args...>)
  explicit TrojanIngress(vo::Ingress const& vo, Args&&... args)
    : stream_{std::forward<Args>(args)...},
      cred_{vo},
      remote_{std::get<vo::TrojanOption>(*vo.opt_).remote_}
  {
  }

public:
  template <typename Socket>
  requires(std::same_as<Stream, stream::Tls<Socket>>)
  explicit TrojanIngress(vo::Ingress const& vo, Socket s)
    : TrojanIngress{vo, stream::tls_context(*vo.tls_), std::move(s)}
  {
  }

  template <typename Socket>
  requires(std::same_as<Stream, stream::Websocket<stream::Tls<Socket>>>)
  explicit TrojanIngress(vo::Ingress const& vo, Socket s)
    : TrojanIngress{
          vo,
          vo.websocket_->path_,
          vo.websocket_->host_.value_or(""),
          stream::tls_context(*vo.tls_),
          std::move(s)
      }
  {
  }

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  Stream     stream_;
  Credential cred_;
  Cache      cache_ = {};
  Endpoint   remote_;
};

template <typename Stream> class TrojanEgress {
public:
  explicit TrojanEgress(vo::Egress const&, IOExecutor const&)
  requires(stream::TLSStream<Stream>);

  explicit TrojanEgress(vo::Egress const&, IOExecutor const&)
  requires(stream::WebsocketStream<Stream>);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<void> connect(Endpoint const&);

private:
  std::string cred_;
  Endpoint    peer_;
  Stream      stream_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_TROJAN_HPP
