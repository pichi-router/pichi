#include <pichi/common/config.hpp>
// Include config.hpp first
#include <algorithm>
#include <array>
#include <boost/asio/detached.hpp>
#include <format>
#include <iostream>
#include <pichi/actor/listener.hpp>
#include <pichi/common/asserts.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>

namespace asio = boost::asio;
namespace ip   = asio::ip;
namespace json = rapidjson;
namespace rngs = std::ranges;

namespace pichi::actor {

Awaitable<void> Listener::listen(std::string_view, ip::tcp::acceptor& ac, std::any&)
{
  while (ac.is_open()) {
    auto [ec, s] = co_await redirect(ac.async_accept(asio::use_awaitable));
    if (ec) {
      if (ec != asio::error::operation_aborted)
        std::clog << std::format("ERROR: {}\n", ec.message());
      break;
    }

    // TODO copy router_ pointer for future use
    co_await switch_to(strand_.get_inner_executor());
    // TODO start session
  }
}

Listener::Listener(IOExecutor ex, RouterPtr const& router)
  : strand_{asio::make_strand(ex)}, router_{router}
{
}

Awaitable<std::string> Listener::get_ingresses() const
{
  co_await switch_to(strand_);
  auto alloc = json::Document::AllocatorType{};
  auto value = json::Value{};
  value.SetObject();
  for (auto&& [name, holder] : ingresses_) {
    value.AddMember(vo::toJson(name, alloc), vo::toJson(holder.vo_, alloc), alloc);
  }
  co_return vo::toString(value);
}

Awaitable<void> Listener::del_ingress(std::string const& name)
{
  co_await switch_to(strand_);
  auto it = ingresses_.find(name);
  if (it != std::end(ingresses_)) ingresses_.erase(name);
}

Awaitable<void> Listener::put_ingress(std::string const& name, std::string_view value)
{
  co_await switch_to(strand_);

  auto ex = strand_.get_inner_executor();
  auto vo = vo::parse<vo::Ingress>(value);

  assertFalse(vo.type_ == AdapterType::DIRECT, PichiError::MISC);
  assertFalse(vo.type_ == AdapterType::REJECT, PichiError::MISC);

  auto it = ingresses_.find(name);
  if (it == std::end(ingresses_)) {
    auto [n, inserted] = ingresses_.try_emplace(name, ex, std::move(vo));
    assertTrue(inserted, PichiError::MISC);
    it = n;
  }
  else
    it->second.reset(ex, std::move(vo));

  for (auto& ac : it->second.acceptors_)
    asio::co_spawn(ex, listen(name, ac, it->second.data_), asio::detached);
}

}  // namespace pichi::actor
