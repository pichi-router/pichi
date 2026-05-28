#ifndef PICHI_SERVICE_BALANCER_HPP
#define PICHI_SERVICE_BALANCER_HPP

#include <boost/asio/execution_context.hpp>
#include <boost/asio/strand.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <pichi/common/coro.hpp>
#include <pichi/vo/ingress.hpp>
#include <random>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pichi::service {

class Balancer;
using BalancerPtr = std::shared_ptr<Balancer>;

namespace balancer {

class Service : public boost::asio::detail::execution_context_service_base<Service> {
private:
  using Balancers = std::unordered_map<std::string, BalancerPtr>;
  using Strand    = std::optional<boost::asio::strand<IOExecutor>>;

  void shutdown() noexcept override;

public:
  Service(boost::asio::execution_context&);

  void initialize(IOExecutor const&);
  void remove(std::string const&);

  Awaitable<BalancerPtr> get(std::string const&);

  Awaitable<void> create(vo::Ingress const&);

private:
  std::once_flag flag_      = {};
  Balancers      balancers_ = {};
  Strand         strand_    = {};
};

extern Service& use_service(IOExecutor const&);

using Strand = boost::asio::strand<IOExecutor>;

class Random {
public:
  explicit Random(IOExecutor const&, vo::TunnelOption const&);

  Awaitable<Endpoint> select();

  void release(Endpoint const&);

private:
  Strand                                strand_;
  std::vector<Endpoint>                 peers_;
  std::mt19937_64                       g_;
  std::uniform_int_distribution<size_t> rand_;
};

class RoundRobin {
public:
  explicit RoundRobin(IOExecutor const&, vo::TunnelOption const&);

  Awaitable<Endpoint> select();

  void release(Endpoint const&);

private:
  Strand                strand_;
  std::vector<Endpoint> peers_;
  size_t                pos_;
};

class LeastConn {
private:
  using Index = std::multimap<size_t, size_t>;
  using Iters = std::unordered_map<size_t, typename Index::iterator>;
  using Peers = std::vector<Endpoint>;

  static Peers initialize(vo::TunnelOption const&);

public:
  explicit LeastConn(IOExecutor const&, vo::TunnelOption const&);

  Awaitable<Endpoint> select();

  void release(Endpoint const&);

private:
  Strand strand_;
  Index  index_;
  Iters  iters_;
  Peers  peers_;
};

}  // namespace balancer

class Balancer {
private:
  using Manner = std::variant<balancer::Random, balancer::RoundRobin, balancer::LeastConn>;

  static Manner initialize(IOExecutor const&, vo::Ingress const&);

public:
  explicit Balancer(IOExecutor const&, vo::Ingress const&);

  Awaitable<Endpoint> select();

  void release(Endpoint const&);

private:
  Manner manner_;
};

extern Awaitable<void>        create_balancer(IOExecutor const&, vo::Ingress const&);
extern void                   remove_balancer(IOExecutor const&, std::string const&);
extern Awaitable<BalancerPtr> get_balancer(IOExecutor const&, std::string const&);

}  // namespace pichi::service

#endif  // PICHI_SERVICE_BALANCER_HPP
