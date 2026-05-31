#include "pichi/common/config.hpp"
#include <boost/asio/execution/context.hpp>
#include <boost/asio/execution_context.hpp>
#include <pichi/adapter/tcp/direct.hpp>
#include <pichi/service/clients.hpp>
#include <pichi/stream/helpers.hpp>

namespace asio = boost::asio;

namespace pichi::adapter::tcp {

Awaitable<void> Direct::connect(Endpoint const& peer)
{
  co_await stream::connect(socket_, peer);

  auto& clients = asio::use_service<service::TcpClients>(
      asio::query(socket_.get_executor(), asio::execution::context)
  );
  clients.insert(socket_.local_endpoint());
}

Awaitable<size_t> Direct::recv(MutableBuffer buf)
{
  co_return co_await stream::read_some(socket_, buf);
}

Awaitable<void> Direct::send(ConstBuffer buf) { co_await stream::write(socket_, buf); }

Awaitable<void> Direct::close()
{
  auto& clients = asio::use_service<service::TcpClients>(
      asio::query(socket_.get_executor(), asio::execution::context)
  );
  clients.remove(socket_.local_endpoint());

  co_await stream::close(socket_);
}

}  // namespace pichi::adapter::tcp
