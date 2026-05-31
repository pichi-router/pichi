#include <pichi/common/asserts.hpp>
#include <pichi/service/mmdb.hpp>

namespace asio = boost::asio;

namespace pichi::service {

Mmdb::Mmdb(asio::execution_context& ctx)
  : asio::detail::execution_context_service_base<Mmdb>{ctx}, db_{}
{
}

void Mmdb::initialize(std::string const& fn)
{
  std::call_once(
      flag_,
      [this](auto&& fn) {
        db_       = MMDB_s{};
        auto stat = MMDB_open(fn.c_str(), MMDB_MODE_MMAP, std::addressof(*db_));
        assertTrue(stat == MMDB_SUCCESS, MMDB_strerror(stat));
      },
      fn
  );
}

void Mmdb::shutdown() noexcept
{
  if (db_.has_value()) MMDB_close(std::addressof(*db_));
  db_.reset();
}

bool Mmdb::match(sockaddr const* const endpoint, std::string_view country)
{
  if (!db_.has_value()) return false;

  auto status = MMDB_SUCCESS;
  auto result = MMDB_lookup_sockaddr(std::addressof(*db_), endpoint, &status);

  if (status != MMDB_SUCCESS || !result.found_entry) return false;

  auto entry = MMDB_entry_data_s{};
  status     = MMDB_get_value(&result.entry, &entry, "country", "iso_code", nullptr);

  if (status != MMDB_SUCCESS || !entry.has_data) return false;
  assertTrue(entry.type == MMDB_DATA_TYPE_UTF8_STRING);

  return std::string_view{entry.utf8_string, entry.data_size} == country;
}

}  // namespace pichi::service
