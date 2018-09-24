#ifndef PICHI_API_INGRESS_MANAGER_HPP
#define PICHI_API_INGRESS_MANAGER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/asio/strand.hpp>
#include <exception>
#include <map>
#include <pichi/api/iterator.hpp>
#include <pichi/api/rest.hpp>
#include <utility>

namespace pichi::api {

class Router;
class Egress;

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
  static void stub(std::exception_ptr);
  static void logging(std::exception_ptr);

  template <typename Function, typename FaultHandler = decltype(stub)>
  void spawn(Function&&, FaultHandler&& = stub);
  void listen(typename Container::iterator, boost::asio::yield_context);

public:
  IngressManager(Strand, Router const&, Egress const&);

  void update(std::string const&, IngressVO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;

private:
  Strand strand_;
  Router const& router_;
  Egress const& egress_;
  Container c_;
};

} // namespace pichi::api

#endif // PICHI_API_INGRESS_MANAGER_HPP
