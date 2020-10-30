#ifndef PICHI_STREAM_TRAITS_HPP
#define PICHI_STREAM_TRAITS_HPP

namespace pichi::stream {

template <typename Stream> struct AsyncStream : public std::true_type {
};

template <typename Stream> struct RawStream : public std::true_type {
};

template <typename Stream> inline constexpr bool IsRawStream = RawStream<Stream>::value;
template <typename Stream> inline constexpr bool IsAsyncStream = AsyncStream<Stream>::value;

}  // namespace pichi::stream

#endif  // PICHI_STREAM_TRAITS_HPP
