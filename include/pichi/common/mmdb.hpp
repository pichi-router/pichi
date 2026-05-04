#ifndef PICHI_COMMON_MMDB_HPP
#define PICHI_COMMON_MMDB_HPP

#include <boost/asio/execution_context.hpp>
#include <maxminddb.h>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace pichi {

class Mmdb : public boost::asio::detail::execution_context_service_base<Mmdb> {
public:
  explicit Mmdb(boost::asio::execution_context&);

  void initialize(std::string const&);

  bool match(sockaddr const* const, std::string_view);

private:
  void shutdown() noexcept override;

  std::mutex            mutex_;
  std::optional<MMDB_s> db_;
};

}  // namespace pichi

#endif  // PICHI_COMMON_MMDB_HPP
