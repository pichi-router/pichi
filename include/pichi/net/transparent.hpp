#ifndef PICHI_NET_TRANSPARENT_HPP
#define PICHI_NET_TRANSPARENT_HPP

#include <pichi/common/adapter.hpp>
#include <utility>

namespace pichi::net {

template <typename Socket> class TransparentIngress : public Ingress {
public:
  template <typename... Args>
  TransparentIngress(Args&&... args) : socket_{std::forward<Args>(args)...}
  {
  }

  ~TransparentIngress() = default;
  TransparentIngress(TransparentIngress const&) = delete;
  TransparentIngress(TransparentIngress&&) = delete;
  TransparentIngress& operator=(TransparentIngress const&) = delete;
  TransparentIngress& operator=(TransparentIngress&&) = delete;

  size_t recv(MutableBuffer, Yield) override;
  void send(ConstBuffer, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;

private:
private:
  Socket socket_;
};

}  // namespace pichi::net

#endif  // PICHI_NET_TRANSPARENT_HPP
