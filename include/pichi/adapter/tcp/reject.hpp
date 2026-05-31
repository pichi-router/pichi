#ifndef PICHI_ADAPTER_TCP_REJECT_HPP
#define PICHI_ADAPTER_TCP_REJECT_HPP

#include <boost/asio/system_timer.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>
#include <pichi/vo/egress.hpp>

namespace pichi::adapter::tcp {

class RejectEgress {
public:
  explicit RejectEgress(vo::Egress const&, IOExecutor const&);

  [[noreturn]] Awaitable<size_t> recv(MutableBuffer);
  [[noreturn]] Awaitable<void>   send(ConstBuffer);
  Awaitable<void>                close();

  Awaitable<void> connect(Endpoint const&);

private:
  boost::asio::system_timer timer_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_REJECT_HPP
