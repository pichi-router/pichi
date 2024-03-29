#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/beast/http/error.hpp>
#include <boost/beast/websocket/error.hpp>
#include <iostream>
#include <pichi/common/error.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace http = boost::beast::http;
namespace sys = boost::system;
namespace ws = boost::beast::websocket;

namespace pichi::net {

void logException(std::exception_ptr eptr) noexcept
{
  try {
    if (eptr) rethrow_exception(eptr);
  }
  catch (sys::system_error& e) {
    if (e.code() == asio::error::eof || e.code() == asio::error::operation_aborted ||
        e.code() == http::error::end_of_stream || e.code() == ws::error::closed)
      return;
    cout << "ERROR: " << e.what() << endl;
  }
}

void stubHandler(std::exception_ptr, asio::yield_context) noexcept {}

}  // namespace pichi::net
