#include <pichi/net/asio.hpp>
#include <pichi/net/common.hpp>
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

void DirectAdapter::close() { pichi::net::close(socket_); }

bool DirectAdapter::readable() const { return isOpen(socket_); }

bool DirectAdapter::writable() const { return isOpen(socket_); }

void DirectAdapter::connect(Endpoint const&, Endpoint const& server, Yield yield)
{
  pichi::net::connect(server, socket_, yield);
}

} // namespace pichi::net
