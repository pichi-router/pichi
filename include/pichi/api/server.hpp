#ifndef PICHI_API_SERVER_HPP
#define PICHI_API_SERVER_HPP

#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <pichi/api/egress_manager.hpp>
#include <pichi/api/ingress_manager.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/api/router.hpp>
#include <pichi/common/buffer.hpp>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace pichi::api {

struct IngressHolder;

class Server {
private:
  using ResolveResult = boost::asio::ip::tcp::resolver::results_type;

  template <typename Yield> void listen(std::string_view, IngressHolder&, Yield yield);
  template <typename ExceptionPtr> void removeIngress(ExceptionPtr, std::string_view);
  vo::EgressVO const& route(Endpoint const&, std::string_view ingress, AdapterType,
                            ResolveResult const&);
  bool isDuplicated(ConstBuffer<uint8_t>);

public:
  Server(Server const&) = delete;
  Server(Server&&) = delete;
  Server& operator=(Server const&) = delete;
  Server& operator=(Server&&) = delete;

  Server(boost::asio::io_context&, char const*);
  ~Server() = default;

  void listen(std::string_view, uint16_t);
  void startIngress(std::string_view, IngressHolder&);

private:
  boost::asio::io_context::strand strand_;
  std::unordered_set<std::string> ivs_;
  Router router_;
  EgressManager egresses_;
  IngressManager ingresses_;
  Rest rest_;
  std::string bind_;
  uint16_t port_;
};

}  // namespace pichi::api

#endif  // PICHI_API_SERVER_HPP
