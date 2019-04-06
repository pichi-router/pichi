#ifndef PICHI_TEST_UTILS_HPP
#define PICHI_TEST_UTILS_HPP

#include <boost/asio/error.hpp>
#include <boost/beast/http/error.hpp>
#include <pichi/api/vos.hpp>
#include <pichi/exception.hpp>
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
  return e.code() == error;
}

extern std::vector<uint8_t> str2vec(std::string_view);
extern std::vector<uint8_t> hex2bin(std::string_view);

inline decltype(auto) ph = "placeholder";
extern rapidjson::Document::AllocatorType& alloc;

extern api::IngressVO defaultIngressVO(api::AdapterType);
extern rapidjson::Value defaultIngressJson(api::AdapterType);
extern api::EgressVO defaultEgressVO(api::AdapterType);
extern rapidjson::Value defaultEgressJson(api::AdapterType);

} // namespace pichi

#endif
