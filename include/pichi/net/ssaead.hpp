#ifndef PICHI_NET_SSAEAD_HPP
#define PICHI_NET_SSAEAD_HPP

#include <boost/beast/core/flat_buffer.hpp>
#include <pichi/common/adapter.hpp>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/method.hpp>

namespace pichi::net {

template <crypto::CryptoMethod method, typename Stream>
class SSAeadAdapter : public pichi::Ingress, public pichi::Egress {
private:
  using Cache = boost::beast::basic_flat_buffer<std::allocator<uint8_t>>;

public:
  inline static constexpr crypto::CryptoMethod METHOD = method;

  template <typename... Args>
  SSAeadAdapter(ConstBuffer<uint8_t> psk, Args&&... args)
    : stream_{std::forward<Args>(args)...}, encryptor_{psk}, decryptor_{psk}
  {
  }
  ~SSAeadAdapter() override = default;

public:
  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  size_t readIV(MutableBuffer<uint8_t>, Yield) override;
  Endpoint readRemote(Yield) override;
  void connect(Endpoint const& remote, ResolveResults next, Yield) override;
  void confirm(Yield) override;

private:
  MutableBuffer<uint8_t> prepare(size_t n, MutableBuffer<uint8_t> provided);
  size_t copyTo(MutableBuffer<uint8_t>);
  void recvBlock(MutableBuffer<uint8_t> block, Yield);
  size_t recvFrame(MutableBuffer<uint8_t>, Yield);
  size_t encrypt(ConstBuffer<uint8_t> plain, MutableBuffer<uint8_t> cipher);

private:
  Stream stream_;
  Cache cache_;
  crypto::AeadEncryptor<method> encryptor_;
  crypto::AeadDecryptor<method> decryptor_;
  bool ivSent_ = false;
  bool ivReceived_ = false;
};

}  // namespace pichi::net

#endif  // PICHI_NET_SSAEAD_HPP
