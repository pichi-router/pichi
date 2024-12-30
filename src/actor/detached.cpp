#include <boost/asio/error.hpp>
#include <boost/system/system_error.hpp>
#include <format>
#include <iostream>
#include <pichi/actor/detached.hpp>

namespace asio = boost::asio;
namespace sys  = boost::system;

namespace pichi::actor {

void Detached::handler(std::exception_ptr ep)
{
  try {
    if (ep) std::rethrow_exception(ep);
  }
  catch (sys::system_error const& e) {
    if (e.code() != asio::error::eof && e.code() != asio::error::operation_aborted)
      std::clog << std::format("Error: {}\n", e.what());
  }
  catch (std::exception const& e) {
    std::clog << std::format("Unexpected exception: {}\n", e.what());
    std::terminate();
  }
  catch (...) {
    std::clog << "Unexpected exception\n";
    std::terminate();
  }
}

}  // namespace pichi::actor
