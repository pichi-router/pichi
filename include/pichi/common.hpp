#ifndef PICHI_COMMON_HPP
#define PICHI_COMMON_HPP

#include <type_traits>

namespace pichi {

/* TODO
 * MSVC complains C4100 warnings if variant is not used in all branches of if-constexpr.
 * Suppress it explicitly by using following function tmeplate.
 */
template <typename T> void suppressC4100(T&& t) { static_assert(std::is_same_v<T, decltype(t)>); }

} // namespace pichi

#endif // PICHI_COMMON_HPP
