#ifndef PICHI_NET_ADAPTER_HPP
#define PICHI_NET_ADAPTER_HPP

#include <boost/asio/spawn2.hpp>
#include <pichi/buffer.hpp>
#include <pichi/exception.hpp>
#include <pichi/net/common.hpp>
#include <stdint.h>

namespace pichi::net {

struct Adapter {
  using Yield = boost::asio::yield_context;

  Adapter(Adapter const&) = delete;
  Adapter(Adapter&&) = delete;
  Adapter& operator=(Adapter const&) = delete;
  Adapter& operator=(Adapter&&) = delete;

  Adapter() = default;
  virtual ~Adapter() = default;

  virtual size_t recv(MutableBuffer<uint8_t>, Yield) = 0;
  virtual void send(ConstBuffer<uint8_t>, Yield) = 0;
  virtual void close(Yield) = 0;
  virtual bool readable() const = 0;
  virtual bool writable() const = 0;
};

struct Ingress : public Adapter {
  virtual size_t readIV(MutableBuffer<uint8_t>, Yield) { return 0; };
  virtual Endpoint readRemote(Yield) = 0;
  virtual void confirm(Yield) = 0;
  virtual void disconnect(PichiError, Yield) = 0;
};

struct Egress : public Adapter {
  virtual void connect(Endpoint const& remote, Endpoint const& server, Yield) = 0;
};

} // namespace pichi::net

#endif // PICHI_NET_ADAPTER_HPP
