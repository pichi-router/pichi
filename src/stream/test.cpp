#include "pichi/common/config.hpp"
#include <boost/asio/connect_pipe.hpp>
#include <pichi/stream/test.hpp>

namespace asio = boost::asio;

namespace pichi::unit_test {

TestSocket::TestSocket(IOExecutor const& ex) : in_{ex}, out_{ex} {}

TestSocket::executor_type TestSocket::get_executor() { return in_.get_executor(); }

bool TestSocket::is_open() const { return in_.is_open() && out_.is_open(); }

void TestSocket::close()
{
  in_.close();
  out_.close();
}

TestSocket TestSocket::peer()
{
  auto ret = TestSocket{get_executor()};
  asio::connect_pipe(in_, ret.out_);
  asio::connect_pipe(ret.in_, out_);
  return ret;
}

}  // namespace pichi::unit_test

namespace boost::beast {

void beast_close_socket(pichi::unit_test::TestSocket& s) { s.close(); }

}  // namespace boost::beast
