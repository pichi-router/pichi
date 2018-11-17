#ifndef PICHI_NET_HTTP_HPP
#define PICHI_NET_HTTP_HPP

#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <pichi/buffer.hpp>
#include <pichi/net/adapter.hpp>
#include <pichi/net/common.hpp>
#include <utility>

namespace pichi::net {

class HttpIngress : public Ingress {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  HttpIngress(Socket&& socket) : socket_{std::move(socket)} {}
  ~HttpIngress() override = default;

public:
  size_t recv(MutableBuffer<uint8_t> buf, Yield yield) override
  {
    return delegate_->recv(buf, yield);
  }

  void send(ConstBuffer<uint8_t> buf, Yield yield) override { delegate_->send(buf, yield); }

  void close() override { delegate_->close(); }

  bool readable() const override { return delegate_->readable(); }

  bool writable() const override { return delegate_->writable(); }

  void confirm(Yield yield) override { delegate_->confirm(yield); }

  Endpoint readRemote(Yield) override;

private:
  Socket socket_;
  std::unique_ptr<Ingress> delegate_ = nullptr;
};

class HttpEgress : public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  HttpEgress(Socket&&);
  ~HttpEgress() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;

private:
  Socket socket_;
};

} // namespace pichi::net

#endif // PICHI_NET_HTTP_HPP
