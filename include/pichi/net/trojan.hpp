#ifndef PICHI_NET_TROJAN_HPP
#define PICHI_NET_TROJAN_HPP

#include <boost/beast/core/flat_buffer.hpp>
#include <memory>
#include <pichi/common/adapter.hpp>
#include <string>
#include <string_view>
#include <unordered_set>

namespace pichi::net {

extern std::string sha224(std::string_view);

template <typename Stream> class TrojanIngress : public Ingress {
public:
  template <typename InputIt, typename... Args>
  TrojanIngress(Endpoint remote, InputIt first, InputIt last, Args&&... args)
    : remote_{remote}, passwords_{}, stream_{std::forward<Args>(args)...}
  {
    std::transform(first, last, std::inserter(passwords_, std::end(passwords_)), &sha224);
  }

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  Endpoint readRemote(Yield) override;
  void confirm(Yield) override;

private:
  Endpoint remote_;
  std::unordered_set<std::string> passwords_;
  Stream stream_;
  boost::beast::flat_buffer buf_ = {};
  std::unique_ptr<Adapter> delegate_ = nullptr;
};

template <typename Stream> class TrojanEgress : public Egress {
public:
  template <typename... Args>
  TrojanEgress(std::string_view password, Args&&... args)
    : stream_{std::forward<Args>(args)...}, password_{sha224(password)}
  {
  }

  size_t recv(MutableBuffer<uint8_t>, Yield) override;
  void send(ConstBuffer<uint8_t>, Yield) override;
  void close(Yield) override;
  bool readable() const override;
  bool writable() const override;
  void connect(Endpoint const& remote, ResolveResults next, Yield) override;

private:
  Stream stream_;
  std::string password_;
};

}  // namespace pichi::net

#endif  // PICHI_NET_TROJAN_HPP
