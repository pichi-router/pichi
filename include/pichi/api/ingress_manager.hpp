#ifndef PICHI_API_INGRESS_MANAGER_HPP
#define PICHI_API_INGRESS_MANAGER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <map>
#include <pichi/api/iterator.hpp>
#include <pichi/api/vos.hpp>
#include <pichi/buffer.hpp>
#include <unordered_set>
#include <utility>

namespace pichi::api {

class Router;
class EgressManager;

class IngressManager {
public:
  using VO = IngressVO;

private:
  using Strand = boost::asio::io_context::strand;
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using Container = std::map<std::string, std::pair<IngressVO, Acceptor>, std::less<>>;
  using DelegateIterator = typename Container::const_iterator;
  using ValueType = std::pair<std::string_view, IngressVO const&>;
  using ConstIterator = Iterator<DelegateIterator, ValueType>;

private:
  static ValueType generatePair(DelegateIterator);

  template <typename Yield>
  std::pair<EgressVO const&, net::Endpoint> route(net::Endpoint const&, std::string_view ingress,
                                                  AdapterType, Yield);
  template <typename Yield> bool isDuplicated(ConstBuffer<uint8_t>, Yield);
  template <typename Yield> void listen(typename Container::iterator, Yield);

public:
  IngressManager(Strand, Router const&, EgressManager const&);

  void update(std::string const&, IngressVO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;

private:
  Strand strand_;
  Router const& router_;
  EgressManager const& eManager_;
  Container c_;
  std::unordered_set<std::string> ivs_;
};

} // namespace pichi::api

#endif // PICHI_API_INGRESS_MANAGER_HPP
