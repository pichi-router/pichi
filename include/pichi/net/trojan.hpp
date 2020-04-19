#ifndef PICHI_NET_TROJAN_HPP
#define PICHI_NET_TROJAN_HPP

#include <optional>
#include <pichi/common.hpp>
#include <pichi/net/adapter.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace pichi::net {

extern std::string sha224(std::string_view);

template <typename Stream> class TrojanIngress : public Ingress {
public:
  template <typename InputIt, typename... Args>
  TrojanIngress(Endpoint remote, InputIt first, InputIt last, Args&&... args)
    : remote_{remote}, passwords_{}, stream_{std::forward<Args>(args)...}, received_(512, '\0')
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
  std::vector<uint8_t> received_;
};

} // namespace pichi::net

#endif // PICHI_NET_TROJAN_HPP
