#ifndef PICHI_SERVICE_MMDB_HPP
#define PICHI_SERVICE_MMDB_HPP

#include <boost/asio/execution_context.hpp>
#include <maxminddb.h>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace pichi::service {

class Mmdb : public boost::asio::detail::execution_context_service_base<Mmdb> {
private:
  using Database = std::optional<MMDB_s>;

public:
  explicit Mmdb(boost::asio::execution_context&);

  void initialize(std::string const&);

  bool match(sockaddr const* const, std::string_view);

private:
  void shutdown() noexcept override;

  std::once_flag flag_;
  Database       db_;
};

}  // namespace pichi::service

#endif  // PICHI_SERVICE_MMDB_HPP
