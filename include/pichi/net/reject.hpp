#ifndef PICHI_NET_REJECT_HPP
#define PICHI_NET_REJECT_HPP

#include <boost/asio/system_timer.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi::net {

class RejectEgress : public Egress {
private:
  using Timer = boost::asio::system_timer;

public:
  explicit RejectEgress(boost::asio::io_context&);
  explicit RejectEgress(boost::asio::io_context&, uint16_t);
  ~RejectEgress() override = default;

  [[noreturn]] size_t recv(MutableBuffer<uint8_t>, Yield) override;
  [[noreturn]] void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  [[noreturn]] bool readable() const override;
  [[noreturn]] bool writable() const override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;

private:
  Timer t_;
};

} // namespace pichi::net

#endif // PICHI_NET_REJECT_HPP
