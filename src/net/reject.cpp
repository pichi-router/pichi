#include <chrono>
#include <pichi/asserts.hpp>
#include <pichi/net/reject.hpp>
#include <random>

using namespace std;
namespace asio = boost::asio;

namespace pichi::net {

RejectEgress::RejectEgress(Socket&& s) : t_{s.get_executor().context()}
{
  auto g = mt19937{random_device{}()};
  auto r = uniform_int_distribution<>{0, 300};
  t_.expires_after(r(g) * 1s);
}

size_t RejectEgress::recv(MutableBuffer<uint8_t>, Yield yield)
{
  assertTrue(running_);
  t_.async_wait(yield);
  fail("Force to reject connection");
}

void RejectEgress::send(ConstBuffer<uint8_t>, Yield) { assertTrue(running_); }

void RejectEgress::close()
{
  if (running_) t_.cancel();
  running_ = false;
}

bool RejectEgress::readable() const { return running_; }

bool RejectEgress::writable() const { return running_; }

void RejectEgress::connect(Endpoint const&, Endpoint const&, Yield) { assertTrue(running_); }

} // namespace pichi::net
