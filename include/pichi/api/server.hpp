#ifndef PICHI_API_SERVER_HPP
#define PICHI_API_SERVER_HPP

#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <pichi/api/egress_manager.hpp>
#include <pichi/api/ingress_manager.hpp>
#include <pichi/api/rest.hpp>
#include <pichi/api/router.hpp>
#include <string_view>

namespace pichi::api {

class Server {
public:
  Server(Server const&) = delete;
  Server(Server&&) = delete;
  Server& operator=(Server const&) = delete;
  Server& operator=(Server&&) = delete;

  Server(boost::asio::io_context&, char const*);
  ~Server() = default;

  void listen(std::string_view, uint16_t);

private:
  boost::asio::io_context::strand strand_;
  Router router_;
  EgressManager egresses_;
  IngressManager ingresses_;
  Rest rest_;
};

} // namespace pichi::api

#endif // PICHI_API_SERVER_HPP
