#include "pichi/common/config.hpp"
#include <boost/asio/post.hpp>
#include <pichi/service/clients.hpp>

namespace asio = boost::asio;

using asio::ip::tcp;

namespace pichi::service {

void TcpClients::shutdown() noexcept
{
  pool_.clear();
  strand_.reset();
}

TcpClients::TcpClients(asio::execution_context& ctx)
  : asio::detail::execution_context_service_base<TcpClients>{ctx}
{
}

void TcpClients::initialize(IOExecutor const& ex)
{
  std::call_once(flag_, [this](auto&& ex) { strand_.emplace(asio::make_strand(ex)); }, ex);
}

void TcpClients::remove(tcp::endpoint const& endpoint)
{
  asio::post(*strand_, [=, this]() { pool_.erase(endpoint); });
}

Awaitable<bool> TcpClients::contains(tcp::endpoint const& endpoint)
{
  co_await switch_to(*strand_);
  co_return pool_.contains(endpoint);
}

void TcpClients::insert(tcp::endpoint const& endpoint)
{
  asio::post(*strand_, [=, this]() { pool_.emplace(endpoint); });
}

}  // namespace pichi::service
