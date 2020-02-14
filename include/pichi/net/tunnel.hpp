#ifndef PICHI_NET_TUNNEL_HPP
#define PICHI_NET_TUNNEL_HPP

#include <pichi/api/balancer.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>

namespace pichi::net {

template <typename Iterator, typename Socket> class TunnelIngress : public Ingress {
private:
  using Balancer = api::Balancer<Iterator>;
  using ValueType = typename std::iterator_traits<Iterator>::value_type;
  static_assert(std::is_same_v<ValueType, Endpoint>);

public:
  template <typename... Args>
  TunnelIngress(Balancer& balancer, Args&&... args)
    : balancer_{balancer}, socket_{std::forward<Args>(args)...}
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

private:
  Balancer& balancer_;
  Socket socket_;
  Iterator it_ = {};
  bool released_ = false;
};

} // namespace pichi::net

#endif // PICHI_NET_TUNNEL_HPP
