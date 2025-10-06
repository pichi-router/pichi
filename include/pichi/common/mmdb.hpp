#ifndef PICHI_COMMON_MMDB_HPP
#define PICHI_COMMON_MMDB_HPP

#include <maxminddb.h>
#include <optional>
#include <string>
#include <string_view>

namespace pichi {

class MMDB {
public:
  MMDB(std::string const&);
  ~MMDB();

  bool match(sockaddr const* const, std::string_view);

private:
  MMDB_s db_ = {};
};

extern std::optional<MMDB> g_mmdb;

}  // namespace pichi

#endif  // PICHI_COMMON_MMDB_HPP
