#ifndef PICHI_ADAPTER_TCP_DIRECT_HPP
#define PICHI_ADAPTER_TCP_DIRECT_HPP

#include <boost/asio/execution/executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <pichi/common/endpoint.hpp>

namespace pichi::adapter::tcp {

class Direct {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  template <boost::asio::execution::executor Executor>
  explicit Direct(Executor const& ex) : socket_{ex}
  {
  }

  Awaitable<void>   connect(Endpoint const&);
  Awaitable<size_t> recv(MutableBuffer);
  Awaitable<void>   send(ConstBuffer);
  Awaitable<void>   close();

private:
  Socket socket_;
};

}  // namespace pichi::adapter::tcp

#endif  // PICHI_ADAPTER_TCP_DIRECT_HPP
