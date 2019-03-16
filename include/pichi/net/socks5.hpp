#ifndef PICHI_NET_SOCKS5_HPP
#define PICHI_NET_SOCKS5_HPP

#include <pichi/net/adapter.hpp>
#include <type_traits>

namespace pichi::net {

template <typename Stream> class Socks5Adapter : public Ingress, public Egress {
public:
  template <typename... Args> Socks5Adapter(Args&&... args) : stream_{std::forward<Args>(args)...}
  {
  }

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Stream stream_;
};

/*
class Socks5Adapter : public Ingress, public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  Socks5Adapter(Socket&&);

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Socket socket_;
};

class Socks5sIngress : public Ingress {
private:
  using Socket = boost::asio::ip::tcp::socket;
  using Stream = boost::asio::ssl::stream<Socket>;

public:
  Socks5sIngress(Socket&&, boost::asio::ssl::context&);

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;

private:
  Stream stream_;
};

class Socks5sEgress : public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;
  using Stream = boost::asio::ssl::stream<Socket>;

public:
  Socks5sEgress(Socket&&, boost::asio::ssl::context&);

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;

private:
  Stream stream_;
};
*/

} // namespace pichi::net

#endif // PICHI_NET_SOCKS5_HPP
