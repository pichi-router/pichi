#include <algorithm>
#include <array>
#include <pichi/asserts.hpp>
#include <pichi/common.hpp>
#include <pichi/crypto/hash.hpp>
#include <sodium/utils.h>

using namespace std;

namespace pichi::crypto {

template <HashAlgorithm algorithm> Hash<algorithm>::Hash()
{
  Traits::initialize(&ctx_);
  assertTrue(Traits::start(&ctx_) == 0, PichiError::MISC);
}

template <HashAlgorithm algorithm> Hash<algorithm>::~Hash() { Traits::release(&ctx_); }

template <HashAlgorithm algorithm> Hash<algorithm>::Hash(Hash const& other)
{
  Traits::clone(&ctx_, &other.ctx_);
}

template <HashAlgorithm algorithm> Hash<algorithm>::Hash(Hash&& other)
{
  Traits::clone(&ctx_, &other.ctx_);
}

template <HashAlgorithm algorithm> void Hash<algorithm>::append(ConstBuffer<uint8_t> src)
{
  if (src.size() == 0) return;
  assertTrue(Traits::update(&ctx_, src.data(), src.size()) == 0, PichiError::MISC);
}

template <HashAlgorithm algorithm> size_t Hash<algorithm>::hash(MutableBuffer<uint8_t> dst)
{
  if (dst.size() < Traits::length) {
    auto tmp = array<uint8_t, Traits::length>{};
    assertTrue(Traits::finish(&ctx_, tmp.data()) == 0, PichiError::MISC);
    copy_n(begin(tmp), dst.size(), begin(dst));
    return dst.size();
  }
  assertTrue(Traits::finish(&ctx_, dst.data()) == 0, PichiError::MISC);
  return Traits::length;
}

template <HashAlgorithm algorithm>
size_t Hash<algorithm>::hash(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  append(src);
  return hash(dst);
}

template class Hash<HashAlgorithm::MD5>;
template class Hash<HashAlgorithm::SHA1>;
template class Hash<HashAlgorithm::SHA224>;
template class Hash<HashAlgorithm::SHA256>;
template class Hash<HashAlgorithm::SHA384>;
template class Hash<HashAlgorithm::SHA512>;

template <HashAlgorithm algorithm> Hmac<algorithm>::Hmac(ConstBuffer<uint8_t> key)
{
  auto k = array<uint8_t, Traits::block_size>{0};
  if (key.size() > Traits::block_size)
    Hash<algorithm>{}.hash(key, k);
  else
    copy_n(begin(key), key.size(), begin(k));

  auto padding = array<uint8_t, Traits::block_size>{0};

  transform(begin(k), end(k), begin(padding), [](auto c) -> uint8_t { return c ^ 0x5c; });
  o_.append(padding);

  transform(begin(k), end(k), begin(padding), [](auto c) -> uint8_t { return c ^ 0x36; });
  i_.append(padding);
}

template <HashAlgorithm algorithm> void Hmac<algorithm>::append(ConstBuffer<uint8_t> src)
{
  i_.append(src);
}

template <HashAlgorithm algorithm> size_t Hmac<algorithm>::hash(MutableBuffer<uint8_t> dst)
{
  auto tmp = array<uint8_t, Traits::length>{};
  i_.hash(tmp);
  return o_.hash(tmp, dst);
}

template <HashAlgorithm algorithm>
size_t Hmac<algorithm>::hash(ConstBuffer<uint8_t> src, MutableBuffer<uint8_t> dst)
{
  append(src);
  return hash(dst);
}

template class Hmac<HashAlgorithm::MD5>;
template class Hmac<HashAlgorithm::SHA1>;
template class Hmac<HashAlgorithm::SHA224>;
template class Hmac<HashAlgorithm::SHA256>;
template class Hmac<HashAlgorithm::SHA384>;
template class Hmac<HashAlgorithm::SHA512>;

template <HashAlgorithm algorithm>
void hkdf(MutableBuffer<uint8_t> okm, ConstBuffer<uint8_t> ikm, ConstBuffer<uint8_t> salt,
          ConstBuffer<uint8_t> info)
{
  auto prk = array<uint8_t, HashTraits<algorithm>::length>{0};
  Hmac<algorithm>{salt}.hash(ikm, prk);
  auto n = okm.size() / HashTraits<algorithm>::length;
  if (okm.size() % HashTraits<algorithm>::length != 0) n++;
  assertTrue(n <= 0xff, PichiError::MISC);

  auto prev = okm.data();
  auto curr = okm.data();
  for (auto i = 1_sz; i <= n; ++i) {
    auto c = static_cast<uint8_t>(i);
    auto hmac = Hmac<algorithm>{prk};
    hmac.append({prev, static_cast<size_t>(curr - prev)});
    hmac.append(info);
    hmac.append({&c, 1});
    prev = curr;
    curr += hmac.hash({curr, static_cast<size_t>(okm.data() + okm.size() - curr)});
  }
}

template void hkdf<HashAlgorithm::MD5>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                       ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);
template void hkdf<HashAlgorithm::SHA1>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                        ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);
template void hkdf<HashAlgorithm::SHA224>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                          ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);
template void hkdf<HashAlgorithm::SHA256>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                          ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);
template void hkdf<HashAlgorithm::SHA384>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                          ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);
template void hkdf<HashAlgorithm::SHA512>(MutableBuffer<uint8_t>, ConstBuffer<uint8_t>,
                                          ConstBuffer<uint8_t>, ConstBuffer<uint8_t>);

string bin2hex(ConstBuffer<uint8_t> bin)
{
  auto hex = string(bin.size() * 2 + 1, '\0');
  sodium_bin2hex(hex.data(), hex.size(), bin.data(), bin.size());
  hex.erase(cbegin(hex) + hex.size() - 1);
  return hex;
}

} // namespace pichi::crypto
