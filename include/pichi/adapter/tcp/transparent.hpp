#ifndef PICHI_ADAPTER_TCP_TRANSPARENT_HPP
#define PICHI_ADAPTER_TCP_TRANSPARENT_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/vo/ingress.hpp>

namespace pichi::adapter::tcp {

class TransparentIngress {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  explicit TransparentIngress(vo::Ingress const&, Socket);

  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

  Awaitable<Endpoint> read_remote();
  Awaitable<void>     confirm();
  Awaitable<void>     disconnect(boost::system::error_code const&);

private:
  Socket socket_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_TRANSPARENT_HPP
