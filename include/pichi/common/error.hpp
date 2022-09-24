#ifndef PICHI_COMMON_ERROR_HPP
#define PICHI_COMMON_ERROR_HPP

#include <boost/system/error_category.hpp>
#include <boost/system/error_code.hpp>
#include <pichi/common/enumerations.hpp>
#include <type_traits>

namespace boost::system {

template <> struct is_error_code_enum<pichi::PichiError> : public std::true_type {
};

}  // namespace boost::system

namespace pichi {

extern boost::system::error_category const& PICHI_CATEGORY;
extern boost::system::error_code make_error_code(PichiError);
extern boost::system::error_code makeErrorCode(PichiError);

}  // namespace pichi

#endif  // PICHI_COMMON_ERROR_HPP
