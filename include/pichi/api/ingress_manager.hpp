#ifndef PICHI_API_INGRESS_MANAGER_HPP
#define PICHI_API_INGRESS_MANAGER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <map>
#include <memory>
#include <pichi/api/balancer.hpp>
#include <pichi/api/iterator.hpp>
#include <pichi/api/vos.hpp>
#include <utility>

namespace pichi::api {

namespace detail {

struct IngressHolder {
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using Iterator = decltype(IngressVO::destinations_)::const_iterator;

  explicit IngressHolder(boost::asio::io_context&, IngressVO&&);
  ~IngressHolder() = default;

  IngressHolder(IngressHolder const&) = delete;
  IngressHolder(IngressHolder&&) = delete;
  IngressHolder& operator=(IngressHolder const&) = delete;
  IngressHolder& operator=(IngressHolder&&) = delete;

  void reset(boost::asio::io_context&, IngressVO&&);

  IngressVO vo_;
  std::unique_ptr<Balancer<Iterator>> balancer_;
  Acceptor acceptor_;
};

} // namespace detail

class IngressManager {
public:
  using VO = IngressVO;
  using IngressHolder = detail::IngressHolder;

private:
  using Container = std::map<std::string, IngressHolder, std::less<>>;
  using DelegateIterator = typename Container::const_iterator;
  using ValueType = std::pair<std::string_view, IngressVO const&>;
  using ConstIterator = Iterator<DelegateIterator, ValueType>;
  using Handler = std::function<void(std::string_view, IngressHolder&)>;

  static ValueType generatePair(DelegateIterator);

public:
  IngressManager(boost::asio::io_context&, Handler);

  void update(std::string const&, IngressVO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;

private:
  boost::asio::io_context& io_;
  Handler onChange_;
  Container c_;
};

} // namespace pichi::api

#endif // PICHI_API_INGRESS_MANAGER_HPP
