#ifndef PICHI_API_INGRESS_MANAGER_HPP
#define PICHI_API_INGRESS_MANAGER_HPP

#include <functional>
#include <map>
#include <pichi/api/balancer.hpp>
#include <pichi/api/ingress_holder.hpp>
#include <pichi/api/iterator.hpp>
#include <pichi/api/vos.hpp>
#include <utility>

namespace pichi::api {

class IngressManager {
public:
  using VO = IngressVO;

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
