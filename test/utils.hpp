#ifndef PICHI_TEST_UTILS_HPP
#define PICHI_TEST_UTILS_HPP

#include <boost/asio/error.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/beast/http/error.hpp>
#include <pichi/api/vos.hpp>
#include <pichi/exception.hpp>
#include <rapidjson/document.h>
#include <string>
#include <string_view>
#include <vector>

namespace pichi {

template <PichiError error> bool verifyException(Exception const& e) { return e.error() == error; }

template <boost::asio::error::basic_errors error>
bool verifyException(boost::system::system_error const& e)
{
  return e.code() == error;
}

template <boost::beast::http::error error>
bool verifyException(boost::system::system_error const& e)
{
  auto expect = boost::beast::http::make_error_code(error);
  auto fact = e.code();
  // FIXME http_error_category equivalence is failed on Windows shared mode
  return expect.value() == fact.value() &&
         std::string_view{expect.category().name()} == std::string_view{fact.category().name()};
}

extern std::vector<uint8_t> str2vec(std::string_view);
extern std::vector<uint8_t> hex2bin(std::string_view);

inline decltype(auto) ph = "placeholder";
extern rapidjson::Document::AllocatorType& alloc;

extern api::IngressVO defaultIngressVO(api::AdapterType);
extern rapidjson::Value defaultIngressJson(api::AdapterType);
extern api::EgressVO defaultEgressVO(api::AdapterType);
extern rapidjson::Value defaultEgressJson(api::AdapterType);

extern bool operator==(net::Endpoint const& lhs, net::Endpoint const& rhs);
extern bool operator==(api::IngressVO const& lhs, api::IngressVO const& rhs);
extern bool operator==(api::EgressVO const& lhs, api::EgressVO const& rhs);
extern bool operator==(api::RuleVO const& lhs, api::RuleVO const& rhs);
extern bool operator==(api::RouteVO const& lhs, api::RouteVO const& rhs);

extern std::string toString(rapidjson::Value const&);
extern std::string toString(api::IngressVO const&);
extern std::string toString(api::EgressVO const&);
extern std::string toString(api::RuleVO const&);
extern std::string toString(api::RouteVO const&);

extern boost::asio::yield_context gYield;

} // namespace pichi

#endif
