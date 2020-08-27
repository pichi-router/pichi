#ifndef PICHI_TEST_UTILS_HPP
#define PICHI_TEST_UTILS_HPP

#include <pichi/common/config.hpp>
// Include config.hpp first
#include <boost/asio/error.hpp>
#include <boost/asio/spawn2.hpp>
#include <boost/beast/http/error.hpp>
#include <pichi/common/exception.hpp>
#include <rapidjson/document.h>
#include <string_view>
#include <vector>

namespace pichi {

namespace vo {

struct Ingress;
struct Egress;

}  // namespace vo

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

extern vo::Ingress defaultIngressVO(AdapterType);
extern rapidjson::Value defaultIngressJson(AdapterType);
extern vo::Egress defaultEgressVO(AdapterType);
extern rapidjson::Value defaultEgressJson(AdapterType);

extern boost::asio::yield_context gYield;

}  // namespace pichi

#endif
