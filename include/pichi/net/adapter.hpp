#ifndef PICHI_NET_ADAPTER_HPP
#define PICHI_NET_ADAPTER_HPP

#include <boost/asio/spawn2.hpp>
#include <pichi/buffer.hpp>
#include <pichi/net/common.hpp>
#include <stdint.h>

namespace pichi {
namespace net {

struct Adapter {
  Adapter(Adapter const&) = delete;
  Adapter(Adapter&&) = delete;
  Adapter& operator=(Adapter const&) = delete;
  Adapter& operator=(Adapter&&) = delete;

  Adapter() = default;
  virtual ~Adapter() = default;

  virtual size_t recv(MutableBuffer<uint8_t>, boost::asio::yield_context) = 0;
  virtual void send(ConstBuffer<uint8_t>, boost::asio::yield_context) = 0;
  virtual void close() = 0;
  virtual bool readable() const = 0;
  virtual bool writable() const = 0;
};

struct Ingress : public Adapter {
  virtual Endpoint readRemote(boost::asio::yield_context) = 0;
  virtual void confirm(boost::asio::yield_context) = 0;
};

struct Egress : public Adapter {
  virtual void connect(Endpoint const& remote, Endpoint const& server,
                       boost::asio::yield_context) = 0;
};

} // namespace net
} // namespace pichi

#endif
