#ifndef PICHI_NET_TUNNEL_HPP
#define PICHI_NET_TUNNEL_HPP

#include <any>
#include <memory>
#include <pichi/api/balancer.hpp>
#include <pichi/common/adapter.hpp>
#include <pichi/common/endpoint.hpp>

namespace pichi::net {

template <typename Socket> class TunnelIngress : public Ingress {
private:
  using BalancerPtr = std::shared_ptr<api::Balancer>;
  using Iterator = api::Balancer::Iterator;

public:
  template <typename... Args>
  TunnelIngress(std::any& data, Args&&... args)
    : pBalancer_{std::any_cast<BalancerPtr>(data)}, socket_{std::forward<Args>(args)...}
  {
  }

  ~TunnelIngress();

  TunnelIngress(TunnelIngress const&) = delete;
  TunnelIngress(TunnelIngress&&) = delete;
  TunnelIngress& operator=(TunnelIngress const&) = delete;
  TunnelIngress& operator=(TunnelIngress&&) = delete;

  size_t recv(MutableBuffer, Yield) override;
  void send(ConstBuffer, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;

private:
  BalancerPtr pBalancer_;
  Socket socket_;
  Iterator it_ = {};
  bool released_ = false;
};

}  // namespace pichi::net

#endif  // PICHI_NET_TUNNEL_HPP
