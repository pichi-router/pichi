#include <optional>
#include <pichi/common/asserts.hpp>
#include <pichi/common/mmdb.hpp>

namespace pichi {

MMDB::MMDB(std::string const& fn)
{
  auto status = MMDB_open(fn.c_str(), MMDB_MODE_MMAP, &db_);
  assertTrue(status == MMDB_SUCCESS, MMDB_strerror(status));
}

MMDB::~MMDB() { MMDB_close(&db_); }

bool MMDB::match(sockaddr const* const endpoint, std::string_view country)
{
  auto status = MMDB_SUCCESS;
  auto result = MMDB_lookup_sockaddr(&db_, endpoint, &status);

  if (status != MMDB_SUCCESS || !result.found_entry) return false;

  auto entry = MMDB_entry_data_s{};
  status     = MMDB_get_value(&result.entry, &entry, "country", "iso_code", nullptr);

  if (status != MMDB_SUCCESS || !entry.has_data) return false;
  assertTrue(entry.type == MMDB_DATA_TYPE_UTF8_STRING);

  return std::string_view{entry.utf8_string, entry.data_size} == country;
}

std::optional<MMDB> g_mmdb{};

}  // namespace pichi
