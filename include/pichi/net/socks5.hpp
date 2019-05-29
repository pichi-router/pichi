#ifndef PICHI_NET_SOCKS5_HPP
#define PICHI_NET_SOCKS5_HPP

#include <pichi/net/adapter.hpp>
#include <type_traits>

namespace pichi::net {

template <typename Stream> class Socks5Adapter : public Ingress, public Egress {
public:
  template <typename... Args> Socks5Adapter(Args&&... args) : stream_{std::forward<Args>(args)...}
  {
  }

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Stream stream_;
};

} // namespace pichi::net

#endif // PICHI_NET_SOCKS5_HPP
