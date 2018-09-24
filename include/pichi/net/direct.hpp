#ifndef PICHI_NET_DIRECT_HPP
#define PICHI_NET_DIRECT_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi {
namespace net {

class DirectAdapter : public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;
  using Yield = boost::asio::yield_context;

public:
  DirectAdapter(Socket&& socket);
  ~DirectAdapter() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const&, Endpoint const&, Yield) override;

private:
  Socket socket_;
};

} // namespace net
} // namespace pichi

#endif
