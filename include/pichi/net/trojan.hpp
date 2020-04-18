#ifndef PICHI_NET_TROJAN_HPP
#define PICHI_NET_TROJAN_HPP

#include <optional>
#include <pichi/common.hpp>
#include <pichi/crypto/hash.hpp>
#include <pichi/net/adapter.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace pichi::net {

template <typename Stream> class TrojanIngress : public Ingress {
private:
  using Credentials = std::unordered_set<std::string>;

public:
  template <typename InputIt, typename... Args>
  TrojanIngress(Endpoint remote, InputIt first, InputIt last, Args&&... args)
    : remote_{remote}, credentials_{}, stream_{std::forward<Args>(args)...}, received_(512, '\0')
  {
    std::transform(first, last, std::inserter(credentials_, std::end(credentials_)),
                   [](auto&& pwd) {
                     auto bin = std::vector<uint8_t>(PWD_LEN / 2, '\0');
                     auto sha224 = crypto::Hash<crypto::HashAlgorithm::SHA224>{};
                     sha224.hash(ConstBuffer<uint8_t>{pwd}, bin);
                     return crypto::bin2hex(bin);
                   });
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
  Credentials credentials_;
  Stream stream_;
  std::vector<uint8_t> received_;

  static constexpr size_t PWD_LEN = crypto::HashTraits<crypto::HashAlgorithm::SHA224>::length * 2;
};

} // namespace pichi::net

#endif // PICHI_NET_TROJAN_HPP
