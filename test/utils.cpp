#include "utils.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/test/unit_test.hpp>
#include <sodium/utils.h>
#include <string.h>

using namespace std;

namespace pichi::unit_test {

static boost::asio::io_context io;
static boost::asio::detail::Pull* pPull = nullptr;
static boost::asio::detail::Push* pPush = nullptr;
static boost::asio::detail::YieldState* pState = nullptr;
boost::asio::yield_context gYield = {io.get_executor(), *pState, *pPush, *pPull};

vector<uint8_t> str2vec(string_view s) { return {cbegin(s), cend(s)}; }

vector<uint8_t> hex2bin(string_view hex)
{
  auto v = vector<uint8_t>(hex.size() / 2, 0);
  sodium_hex2bin(v.data(), v.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr);
  return v;
}

}  // namespace pichi::unit_test
