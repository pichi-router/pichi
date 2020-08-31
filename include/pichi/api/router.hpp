#ifndef PICHI_API_ROUTER_HPP
#define PICHI_API_ROUTER_HPP

#include <map>
#include <memory>
#include <pichi/vo/iterator.hpp>
#include <pichi/vo/route.hpp>
#include <pichi/vo/rule.hpp>

struct MMDB_s;

namespace boost::asio::ip {

class tcp;
template <typename Protocol> class basic_endpoint;
template <typename Protocol> class basic_resolver_results;

}  // namespace boost::asio::ip

namespace pichi {

struct Endpoint;

namespace api {

extern bool matchPattern(std::string_view remote, std::string_view pattern);
extern bool matchDomain(std::string_view subdomain, std::string_view domain);

class Geo {
public:
  Geo(char const* fn);
  ~Geo();

  Geo(Geo&&) noexcept = default;
  Geo& operator=(Geo&&) noexcept = default;

  Geo(Geo const&) = delete;
  Geo& operator=(Geo const&) = delete;

public:
  bool match(boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> const&,
             std::string_view country) const;

private:
  std::unique_ptr<MMDB_s> db_;
};

class Router {
public:
  using VO = vo::Rule;

private:
  using ResolvedResult = boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp>;
  using Matcher =
      std::function<bool(Endpoint const&, ResolvedResult const&, std::string_view, AdapterType)>;
  using Container = std::map<std::string, std::pair<VO, std::vector<Matcher>>, std::less<>>;
  using DelegateIterator = typename Container::const_iterator;
  using ValueType = std::pair<std::string_view, VO const&>;
  using ConstIterator = vo::Iterator<DelegateIterator, ValueType>;

  static ValueType generatePair(DelegateIterator);

public:
  Router(char const* fn);

  std::string_view route(Endpoint const&, std::string_view ingress, AdapterType,
                         ResolvedResult const&) const;

  void update(std::string const&, VO);
  void erase(std::string_view);

  ConstIterator begin() const noexcept;
  ConstIterator end() const noexcept;
  bool isUsed(std::string_view) const;
  bool needResloving() const;

  vo::Route getRoute() const;
  void setRoute(vo::Route);

private:
  Geo geo_;
  Container rules_ = {};
  bool needResolving_ = false;
  vo::Route route_ = {"direct"};
};

}  // namespace api
}  // namespace pichi

#endif  // PICHI_API_ROUTER_HPP
