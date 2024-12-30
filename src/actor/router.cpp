#include <pichi/common/config.hpp>
// Include config.hpp first
#include <algorithm>
#include <pichi/actor/router.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <ranges>

using namespace std::literals;
namespace asio = boost::asio;
namespace json = rapidjson;
namespace rngs = std::ranges;
namespace sys  = boost::system;

namespace pichi::actor {

static auto const DEFAULT_EGRESS_NAME = "direct"s;

template <rngs::range Range> std::string to_string(Range const& range)
{
  auto alloc = json::Document::AllocatorType{};
  return vo::toString(vo::toJson(rngs::cbegin(range), rngs::cend(range), alloc));
}

Router::Router(IOExecutor ex) : 
  ex_{std::move(ex)},
  egresses_{{DEFAULT_EGRESS_NAME, vo::Egress{.type_ = AdapterType::DIRECT}}},
  route_{.default_ = DEFAULT_EGRESS_NAME}
{
}

std::string Router::get_egresses() const { return to_string(egresses_); }

Router::Pointer Router::del_egress(std::string const& name)
{
  // TODO check if locked
  auto rt = std::make_shared<Router>(*this);
  rt->egresses_.erase(name);
  return rt;
}

Router::Pointer Router::put_egress(std::string const& name, std::string_view value)
{
  auto vo = vo::parse<vo::Egress>(value);
  auto rt = std::make_shared<Router>(*this);

  rt->egresses_[name] = std::move(vo);
  return rt;
}

std::string Router::get_rules() const { return to_string(rules_); }

Router::Pointer Router::del_rule(std::string const& name)
{
  // TODO check if locked
  auto rt = std::make_shared<Router>(*this);
  rt->rules_.erase(name);
  return rt;
}

Router::Pointer Router::put_rule(std::string const& name, std::string_view value)
{
  auto vo = vo::parse<vo::Rule>(value);
  auto rt = std::make_shared<Router>(*this);

  rt->rules_[name] = std::move(vo);
  return rt;
}

std::string Router::get_route() const
{
  auto alloc = json::Document::AllocatorType{};
  return vo::toString(vo::toJson(route_, alloc));
}

Router::Pointer Router::put_route(std::string_view value)
{
  auto vo = vo::parse<vo::Route>(value);
  auto rt = std::make_shared<Router>(*this);

  rt->route_ = std::move(vo);
  return rt;
}

Awaitable<vo::Egress> Router::route(Endpoint const&) const
{
  co_return vo::Egress{.type_ = AdapterType::DIRECT};
}

}  // namespace pichi::actor
