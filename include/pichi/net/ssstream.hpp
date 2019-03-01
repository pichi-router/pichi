#ifndef PICHI_NET_SSSTREAM_HPP
#define PICHI_NET_SSSTREAM_HPP

#include <boost/asio/ip/tcp.hpp>
#include <pichi/crypto/method.hpp>
#include <pichi/crypto/stream.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi::net {

template <crypto::CryptoMethod method> class SSStreamAdapter : public Ingress, public Egress {
private:
  using Socket = boost::asio::ip::tcp::socket;

public:
  SSStreamAdapter(Socket&&, ConstBuffer<uint8_t> psk);
  ~SSStreamAdapter() override = default;

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  size_t readIV(MutableBuffer<uint8_t>, Yield) override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& server, Yield) override;

private:
  Socket socket_;
  crypto::StreamEncryptor<method> encryptor_;
  crypto::StreamDecryptor<method> decryptor_;
  bool ivSent_ = false;
  bool ivReceived_ = false;
};

} // namespace pichi::net

#endif // PICHI_NET_SSSTREAM_HPP
