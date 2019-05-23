#ifndef PICHI_NET_SOCKS5_HPP
#define PICHI_NET_SOCKS5_HPP

#include <optional>
#include <pichi/common.hpp>
#include <pichi/net/adapter.hpp>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace pichi::net {

namespace detail {

template <typename First, typename Second> struct Helper {
  template <typename T>
  inline constexpr static bool IsFirst = std::is_same_v<First, std::decay_t<T>> ||
                                         std::is_same_v<std::optional<First>, std::decay_t<T>>;

  template <typename Arg0, typename... Args> static auto constructFirst(Arg0&& arg0, Args&&...)
  {
    if constexpr (IsFirst<Arg0>) {
      return std::optional<First>{std::forward<Arg0>(arg0)};
    }
    else {
      suppressC4100(std::forward<Arg0>(arg0));
      return std::optional<First>{};
    }
  }

  template <typename Arg0, typename... Args>
  static auto constructSecond(Arg0&& arg0, Args&&... args)
  {
    if constexpr (IsFirst<Arg0>) {
      suppressC4100(std::forward<Arg0>(arg0));
      return Second{std::forward<Args>(args)...};
    }
    else {
      return Second{std::forward<Arg0>(arg0), std::forward<Args>(args)...};
    }
  }
};

} // namespace detail

template <typename Stream> class Socks5Ingress : public Ingress {
private:
  using Credentials = std::unordered_map<std::string, std::string>;
  using Constructor = detail::Helper<Credentials, Stream>;

  void authenticate(Yield);

public:
  template <typename... Args>
  Socks5Ingress(Args&&... args)
    : stream_{Constructor::constructSecond(std::forward<Args>(args)...)},
      credentials_{Constructor::constructFirst(std::forward<Args>(args)...)}
  {
  }

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Stream stream_;
  std::optional<Credentials> credentials_;
};

template <typename Stream> class Socks5Egress : public Egress {
private:
  using Credential = std::pair<std::string, std::string>;
  using Constructor = detail::Helper<Credential, Stream>;

  void authenticate(Yield);

public:
  template <typename... Args>
  Socks5Egress(Args&&... args)
    : stream_{Constructor::constructSecond(std::forward<Args>(args)...)},
      credential_{Constructor::constructFirst(std::forward<Args>(args)...)}
  {
  }

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;

private:
  Stream stream_;
  std::optional<Credential> credential_;
};

} // namespace pichi::net

#endif // PICHI_NET_SOCKS5_HPP
