#ifndef PICHI_TEST_UTILS_HPP
#define PICHI_TEST_UTILS_HPP

#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/error.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/beast/http/error.hpp>
#include <pichi/common/error.hpp>
#include <string_view>
#include <vector>

namespace pichi::unit_test {

using SystemError = boost::system::system_error;

template <PichiError error> bool verifyException(SystemError const& e) { return e.code() == error; }

template <boost::asio::error::basic_errors error> bool verifyException(SystemError const& e)
{
  return e.code() == error;
}

template <boost::beast::http::error error> bool verifyException(SystemError const& e)
{
  auto expect = boost::beast::http::make_error_code(error);
  auto fact = e.code();
  // FIXME http_error_category equivalence is failed on Windows shared mode
  return expect.value() == fact.value() &&
         std::string_view{expect.category().name()} == std::string_view{fact.category().name()};
}

extern std::vector<uint8_t> str2vec(std::string_view);
extern std::vector<uint8_t> hex2bin(std::string_view);

inline decltype(auto) ph = "placeholder";
extern boost::asio::yield_context gYield;

}  // namespace pichi::unit_test

#endif
