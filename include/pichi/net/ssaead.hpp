#ifndef PICHI_NET_SSAEAD_HPP
#define PICHI_NET_SSAEAD_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/method.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi {
namespace net {

template <crypto::CryptoMethod method> class SSAeadAdapter : public Ingress, public Outbound {
private:
  using Socket = boost::asio::ip::tcp::socket;
  using Yield = boost::asio::yield_context;
  using Cache = boost::beast::basic_flat_buffer<std::allocator<uint8_t>>;

public:
  SSAeadAdapter(Socket&&, ConstBuffer<uint8_t> psk);
  ~SSAeadAdapter() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close() override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& next, Yield) override;
  void confirm(Yield) override;

private:
  MutableBuffer<uint8_t> prepare(size_t n, MutableBuffer<uint8_t> provided);
  size_t copyTo(MutableBuffer<uint8_t>);
  void recvBlock(MutableBuffer<uint8_t> block, Yield);
  size_t recvFrame(MutableBuffer<uint8_t>, Yield);
  size_t encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher);

private:
  Socket socket_;
  Cache cache_;
  crypto::AeadEncryptor<method> encryptor_;
  crypto::AeadDecryptor<method> decryptor_;
  bool ivSent_ = false;
  bool ivReceived_ = false;
};

} // namespace net
} // namespace pichi

#endif
