#include <boost/beast/http/error.hpp>
#include <iostream>
#include <pichi/exception.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace sys = boost::system;

namespace pichi::net {

void logException(std::exception_ptr eptr) noexcept
{
  try {
    if (eptr) rethrow_exception(eptr);
  }
  catch (Exception& e) {
    cout << "Pichi Error: " << e.what() << endl;
  }
  catch (sys::system_error& e) {
    if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted ||
        e.code() == http::error::end_of_stream)
      return;
    cout << "Socket Error: " << e.what() << endl;
  }
}

void stubHandler(std::exception_ptr, asio::yield_context) noexcept {}

} // namespace pichi::net
