#ifndef PICHI_NET_SSSTREAM_HPP
#define PICHI_NET_SSSTREAM_HPP

#include <pichi/crypto/method.hpp>
#include <pichi/crypto/stream.hpp>
#include <pichi/net/adapter.hpp>

namespace pichi::net {

template <crypto::CryptoMethod method, typename Stream>
class SSStreamAdapter : public Ingress, public Egress {
public:
  inline static constexpr crypto::CryptoMethod METHOD = method;

  template <typename... Args>
  SSStreamAdapter(ConstBuffer<uint8_t> psk, Args&&... args)
    : stream_{std::forward<Args>(args)...}, encryptor_{psk}, decryptor_{psk}
  {
  }
  ~SSStreamAdapter() override = default;

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  size_t readIV(MutableBuffer<uint8_t>, Yield) override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;
  void disconnect(Yield) override;
  void connect(Endpoint const& remote, Endpoint const& server, Yield) override;

private:
  Stream stream_;
  crypto::StreamEncryptor<method> encryptor_;
  crypto::StreamDecryptor<method> decryptor_;
  bool ivSent_ = false;
  bool ivReceived_ = false;
};

} // namespace pichi::net

#endif // PICHI_NET_SSSTREAM_HPP
