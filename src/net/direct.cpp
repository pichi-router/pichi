#include <pichi/common/config.hpp>
// Include config.hpp first
#include <pichi/net/asio.hpp>
#include <pichi/net/direct.hpp>
#include <utility>

using namespace std;
namespace asio = boost::asio;

namespace pichi::net {

DirectAdapter::DirectAdapter(asio::io_context& io) : socket_{io} {}

size_t DirectAdapter::recv(MutableBuffer<uint8_t> buf, Yield yield)
{
  return readSome(socket_, buf, yield);
}

void DirectAdapter::send(ConstBuffer<uint8_t> buf, Yield yield) { write(socket_, buf, yield); }

void DirectAdapter::close(Yield yield) { pichi::net::close(socket_, yield); }

bool DirectAdapter::readable() const { return socket_.is_open(); }

bool DirectAdapter::writable() const { return socket_.is_open(); }

void DirectAdapter::connect(Endpoint const&, ResolveResults next, Yield yield)
{
  pichi::net::connect(next, socket_, yield);
}

}  // namespace pichi::net
