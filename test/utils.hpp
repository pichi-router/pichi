#ifndef PICHI_TEST_UTILS_HPP
#define PICHI_TEST_UTILS_HPP

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/error.hpp>
#include <string_view>

namespace pichi::unit_test {

using SystemError = boost::system::system_error;

template <PichiError error> bool verify_exception(SystemError const& e)
{
  return e.code() == error;
}

template <boost::asio::error::basic_errors error> bool verify_exception(SystemError const& e)
{
  return e.code() == error;
}

template <boost::beast::http::error error> bool verify_exception(SystemError const& e)
{
  auto expect = boost::beast::http::make_error_code(error);
  auto fact   = e.code();
  // FIXME http_error_category equivalence is failed on Windows shared mode
  return expect.value() == fact.value() &&
         std::string_view{expect.category().name()} == std::string_view{fact.category().name()};
}

template <boost::asio::error::misc_errors error> auto verify_exception(SystemError const& e)
{
  return e.code() == error;
}

template <typename TestCase> void run_case(TestCase&& test)
{
  auto pool = boost::asio::thread_pool{1};
  boost::asio::co_spawn(
      pool,
      std::invoke(std::forward<TestCase>(test), pool.get_executor()),
      [&](auto&& eptr, auto&&...) {
        BOOST_CHECK(!eptr);
        pool.stop();
      }
  );
  pool.join();
}

template <PichiError error> bool verifyException(SystemError const& e) { return e.code() == error; }

template <boost::asio::error::basic_errors error> bool verifyException(SystemError const& e)
{
  return e.code() == error;
}

template <boost::beast::http::error error> bool verifyException(SystemError const& e)
{
  auto expect = boost::beast::http::make_error_code(error);
  auto fact   = e.code();
  // FIXME http_error_category equivalence is failed on Windows shared mode
  return expect.value() == fact.value() &&
         std::string_view{expect.category().name()} == std::string_view{fact.category().name()};
}

}  // namespace pichi::unit_test

#endif
