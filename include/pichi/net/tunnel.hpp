#ifndef PICHI_NET_TUNNEL_HPP
#define PICHI_NET_TUNNEL_HPP

#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>

namespace pichi::net {

template <typename Socket> class TunnelIngress : public Ingress {
public:
  template <typename... Args>
  TunnelIngress(Endpoint endpoint, Args&&... args)
    : endpoint_{std::move(endpoint)}, socket_{std::forward<Args>(args)...}
  {
  }

  ~TunnelIngress();

  TunnelIngress(TunnelIngress const&) = delete;
  TunnelIngress(TunnelIngress&&) = delete;
  TunnelIngress& operator=(TunnelIngress const&) = delete;
  TunnelIngress& operator=(TunnelIngress&&) = delete;

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Endpoint endpoint_;
  Socket socket_;
};

} // namespace pichi::net

#endif // PICHI_NET_TUNNEL_HPP
