#ifndef PICHI_NET_REJECT_HPP
#define PICHI_NET_REJECT_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/system_timer.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi::net {

class RejectEgress : public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;
  using Timer = boost::asio::system_timer;

public:
  explicit RejectEgress(Socket&&);
  ~RejectEgress() override = default;

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;

private:
  bool running_ = true;
  Timer t_;
};

} // namespace pichi::net

#endif // PICHI_NET_REJECT_HPP
