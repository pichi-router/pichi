#include <chrono>
#include <pichi/asserts.hpp>
#include <pichi/net/reject.hpp>
#include <random>

using namespace std;
namespace asio = boost::asio;
namespace sys = boost::system;

namespace pichi::net {

RejectEgress::RejectEgress(asio::io_context& io) : t_{io}
{
  auto g = mt19937{random_device{}()};
  auto r = uniform_int_distribution<>{0, 300};
  t_.expires_after(r(g) * 1s);
}

RejectEgress::RejectEgress(asio::io_context& io, uint16_t delay) : t_{io}
{
  t_.expires_after(delay * 1s);
}

[[noreturn]] size_t RejectEgress::recv(MutableBuffer<uint8_t>, Yield)
{
  fail("RejectEgress::recv shouldn't be invoked");
}

[[noreturn]] void RejectEgress::send(ConstBuffer<uint8_t>, Yield)
{
  fail("RejectEgress::send shouldn't be invoked");
}

void RejectEgress::close() { t_.cancel(); }

[[noreturn]] bool RejectEgress::readable() const
{
  fail("RejectEgress::readable shouldn't be invoked");
}

[[noreturn]] bool RejectEgress::writable() const
{
  fail("RejectEgress::writable shouldn't be invoked");
}

void RejectEgress::connect(Endpoint const&, Endpoint const&, Yield yield)
{
  auto ec = sys::error_code{};
  t_.async_wait(yield[ec]);
  if (ec != asio::error::operation_aborted) fail("Force to reject connection");
}

} // namespace pichi::net
