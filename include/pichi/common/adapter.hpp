#ifndef PICHI_COMMON_ADAPTER_HPP
#define PICHI_COMMON_ADAPTER_HPP

#include <boost/asio/spawn2.hpp>
#include <exception>
#include <pichi/common/buffer.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/enumerations.hpp>
#include <stdint.h>

namespace boost::asio::ip {

class tcp;
template <typename InternetProtocol> class basic_resolver_results;

}  // namespace boost::asio::ip

namespace pichi {

using ResolveResults = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;

struct Adapter {
  using Yield = boost::asio::yield_context;

  Adapter(Adapter const&) = delete;
  Adapter(Adapter&&) = delete;
  Adapter& operator=(Adapter const&) = delete;
  Adapter& operator=(Adapter&&) = delete;

  Adapter() = default;
  virtual ~Adapter() = default;

  virtual size_t recv(MutableBuffer, Yield) = 0;
  virtual void send(ConstBuffer, Yield) = 0;
  virtual void close(Yield) = 0;
  virtual bool readable() const = 0;
  virtual bool writable() const = 0;
};

struct Ingress : public Adapter {
  virtual size_t readIV(MutableBuffer, Yield) { return 0; };
  virtual Endpoint readRemote(Yield) = 0;
  virtual void confirm(Yield) = 0;
  virtual void disconnect(std::exception_ptr, Yield) {}
};

struct Egress : public Adapter {
  virtual void connect(Endpoint const& remote, ResolveResults next, Yield) = 0;
};

}  // namespace pichi

#endif  // PICHI_COMMON_ADAPTER_HPP
