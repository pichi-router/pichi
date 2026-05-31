#ifndef PICHI_SERVICE_CLIENTS_HPP
#define PICHI_SERVICE_CLIENTS_HPP

#include <boost/asio/execution_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <mutex>
#include <optional>
#include <pichi/common/coro.hpp>
#include <unordered_set>

namespace pichi::service {

class TcpClients : public boost::asio::detail::execution_context_service_base<TcpClients> {
private:
  using Pool   = std::unordered_set<boost::asio::ip::tcp::endpoint>;
  using Strand = std::optional<boost::asio::strand<IOExecutor>>;

  void shutdown() noexcept override;

public:
  explicit TcpClients(boost::asio::execution_context&);

  void initialize(IOExecutor const&);

  void insert(boost::asio::ip::tcp::endpoint const&);
  void remove(boost::asio::ip::tcp::endpoint const&);

  Awaitable<bool> contains(boost::asio::ip::tcp::endpoint const&);

private:
  std::once_flag flag_;

  Strand strand_;
  Pool   pool_;
};

}  // namespace pichi::service

#endif  // PICHI_SERVICE_CLIENTS_HPP
