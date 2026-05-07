#ifndef PICHI_ADAPTER_TCP_SOCKS5_HPP
#define PICHI_ADAPTER_TCP_SOCKS5_HPP

#include <array>
#include <boost/system/error_code.hpp>
#include <pichi/stream/concepts.hpp>
#include <pichi/stream/tls.hpp>
#include <pichi/vo/egress.hpp>
#include <pichi/vo/ingress.hpp>
#include <variant>

namespace pichi::adapter::tcp {

namespace socks5 {

class IngressCredential {
public:
  explicit IngressCredential(vo::Ingress const&);

  bool authenticate(std::string const&, std::string const&) const;
  bool need_auth() const;

private:
  std::unordered_map<std::string, std::string> data_;
};

class EgressCredential {
public:
  explicit EgressCredential(vo::Egress const&);

  bool        need_auth() const;
  ConstBuffer data() const;

private:
  std::array<uint8_t, 1024> data_ = {};
  size_t                    len_  = 0;
};

}  // namespace socks5

template <stream::AsyncSocket Socket> class Socks5Ingress {
private:
  using Credential = socks5::IngressCredential;
  using Stream     = std::variant<stream::Tls<Socket>, Socket>;

  Awaitable<void> authenticate();

public:
  explicit Socks5Ingress(vo::Ingress const&, Socket);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  Stream     stream_;
  Credential credential_;
};

template <stream::AsyncSocket Socket> class Socks5Egress {
private:
  using Stream = std::variant<stream::Tls<Socket>, Socket>;

public:
  explicit Socks5Egress(vo::Egress const&, IOExecutor const&);
  explicit Socks5Egress(vo::Egress const&, Socket);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<void> connect(Endpoint const&);

private:
  Stream   stream_;
  Endpoint peer_;

  socks5::EgressCredential credential_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_SOCKS5_HPP
