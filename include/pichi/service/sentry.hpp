#ifndef PICHI_SERVICE_SENTRY_HPP
#define PICHI_SERVICE_SENTRY_HPP

#include <boost/asio/execution_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>
#include <mutex>
#include <optional>
#include <pichi/common/buffer.hpp>
#include <pichi/common/coro.hpp>
#include <string>
#include <unordered_set>

namespace pichi::service {

class SaltSentry : public boost::asio::detail::execution_context_service_base<SaltSentry> {
private:
  using Salts  = std::unordered_set<std::string>;
  using Strand = std::optional<boost::asio::strand<IOExecutor>>;
  using Timer  = std::optional<boost::asio::system_timer>;

  Awaitable<void> run();

public:
  explicit SaltSentry(boost::asio::execution_context&);

  void initialize(IOExecutor const&);

  Awaitable<bool> contains(ConstBuffer);
  Awaitable<void> reset();

  void cancel();

private:
  void shutdown() noexcept override;

  std::once_flag flag_ = {};

  Strand strand_   = {};
  Timer  timer_    = {};
  Salts  expiring_ = {};
  Salts  active_   = {};
};

extern SaltSentry& get_sentry(IOExecutor const&);

}  // namespace pichi::service

#endif  // PICHI_SERVICE_SENTRY_HPP
