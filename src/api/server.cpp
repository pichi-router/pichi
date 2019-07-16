#include <pichi/config.hpp>

#ifdef NO_RETURN_STD_MOVE_FOR_BOOST_ASIO
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-std-move"
#endif // __clang__
#include <boost/asio/ip/basic_resolver.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__
#endif // NO_RETURN_STD_MOVE_FOR_BOOST_ASIO

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <pichi/api/server.hpp>
#include <pichi/api/session.hpp>
#include <pichi/api/vos.hpp>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
#include <pichi/net/asio.hpp>
#include <pichi/net/helpers.hpp>
#include <pichi/net/spawn.hpp>

using namespace std;
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace ip = asio::ip;
namespace sys = boost::system;
using tcp = ip::tcp;

namespace pichi::api {

static auto const IMMEDIATE_REJECTOR =
    EgressVO{AdapterType::REJECT, {}, {}, {}, {}, DelayMode::FIXED, 0_u16};
static auto const RANDOM_REJECTOR =
    EgressVO{AdapterType::REJECT, {}, {}, {}, {}, DelayMode::RANDOM};
static auto const IV_EXPIRE_TIME = 1h;

static auto resolve(net::Endpoint const& remote, asio::io_context& io, asio::yield_context yield)
{
  auto ec = sys::error_code{};
  auto r = tcp::resolver{io}.async_resolve(remote.host_, remote.port_, yield[ec]);
  return ec ? tcp::resolver::results_type{} : r;
}

static auto visitingSelf(tcp::resolver::results_type const& r, string_view bind, uint16_t port)
{
  return cend(r) != find_if(cbegin(r), cend(r), [=](auto&& entry) {
           return entry.endpoint().address() == ip::make_address(bind) &&
                  entry.endpoint().port() == port;
         });
}

Server::Server(asio::io_context& io, char const* fn)
  : strand_{io}, router_{fn}, egresses_{}, ingresses_{io,
                                                      [this](auto&& a, auto in, auto& vo) {
                                                        startIngress(a, in, vo);
                                                      }},
    rest_{ingresses_, egresses_, router_}
{
}

void Server::listen(string_view address, uint16_t port)
{
  bind_ = address;
  port_ = port;
  net::spawn(strand_, [a = tcp::acceptor{strand_.context(), {ip::make_address(address), port}},
                       this](auto yield) mutable {
    while (a.is_open()) {
      auto s = make_shared<tcp::socket>(a.async_accept(yield));
      net::spawn(
          strand_,
          [this, s](auto yield) mutable {
            auto buf = beast::flat_buffer{};
            auto req = Rest::Request{};
            http::async_read(*s, buf, req, yield);

            auto resp = rest_.handle(req);
            http::async_write(*s, resp, yield);
          },
          [s](auto eptr, auto yield) noexcept {
            auto ec = sys::error_code{};
            auto resp = Rest::errorResponse(eptr);
            http::async_write(*s, resp, yield[ec]);
            if (ec) cout << "Ignoring HTTP error: " << ec.message() << endl;
          });
    }
  });
}

template <typename Yield>
void Server::listen(Acceptor& acceptor, string_view iname, IngressVO const& vo, Yield yield)
{
  while (acceptor.is_open()) {
    auto ingress = net::makeIngress(vo, acceptor.async_accept(yield));
    auto p = ingress.get();
    // FIXME Not copying for ingress because net::spawn ensures that
    //   Function and ExceptionHandler share the same scope.
    net::spawn(
        strand_,
        [ingress = move(ingress), &vo, iname, this](auto yield) mutable {
          auto& io = strand_.context();
          auto iv = array<uint8_t, 32>{};
          if (isDuplicated({iv, ingress->readIV(iv, yield)})) {
            make_shared<Session>(io, move(ingress), net::makeEgress(RANDOM_REJECTOR, io))->start();
          }
          else {
            auto remote = ingress->readRemote(yield);

            // Refuse connection to the server itself via ingresses.
            // TODO it might be a rule rather than hardcode.
            auto r = resolve(remote, io, yield);
            auto&& evo = visitingSelf(r, bind_, port_) ? IMMEDIATE_REJECTOR :
                                                         route(remote, iname, vo.type_, r);

            auto session = make_shared<Session>(io, move(ingress), net::makeEgress(evo, io));
            if (evo.type_ == AdapterType::DIRECT || evo.type_ == AdapterType::REJECT)
              session->start(remote);
            else
              session->start(remote, net::makeEndpoint(*evo.host_, *evo.port_));
          }
        },
        [ingress = p](auto eptr, auto yield) noexcept {
          try {
            if (eptr) rethrow_exception(eptr);
          }
          catch (Exception const& e) {
            ingress->disconnect(e.error(), yield);
          }
          catch (...) {
            ingress->disconnect(PichiError::MISC, yield);
          }
        });
  }
}

template <typename ExceptionPtr> void Server::removeIngress(ExceptionPtr eptr, string_view iname)
{
  try {
    rethrow_exception(eptr);
  }
  catch (sys::system_error const& e) {
    if (e.code() != asio::error::operation_aborted) {
      ingresses_.erase(iname);
    }
  }
}

bool Server::isDuplicated(ConstBuffer<uint8_t> raw)
{
  if (raw.size() == 0) return false;

  auto [it, inserted] = ivs_.insert({raw.cbegin(), raw.cend()});
  if (!inserted) {
    cout << "Pichi Error: Duplicated IV" << endl;
    return true;
  }
  net::spawn(strand_, [it = it, this](auto yield) {
    // Exceptions prohibited
    auto ec = sys::error_code{};
    asio::system_timer{strand_.context(), IV_EXPIRE_TIME}.async_wait(yield[ec]);
    ivs_.erase(it);
  });
  return false;
}

EgressVO const& Server::route(net::Endpoint const& remote, string_view iname, AdapterType type,
                              ResolveResult const& r)
{
  auto it = egresses_.find(router_.route(remote, iname, type, r));
  assertFalse(it == cend(egresses_));
  return it->second;
}

void Server::startIngress(Acceptor& acceptor, string_view iname, IngressVO const& vo)
{
  /*
   * IngressVO named `iname` has already been inserted into `ingresses_`.
   * It should be removed if exception occurs.
   */
  net::spawn(
      strand_, [this, &acceptor, iname, &vo](auto yield) { listen(acceptor, iname, vo, yield); },
      [ this, iname ](auto eptr, auto) noexcept { removeIngress(eptr, iname); });
}

} // namespace pichi::api
