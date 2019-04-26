#define BOOST_TEST_MODULE pichi hkdf test

#include "utils.hpp"
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common.hpp>
#include <pichi/crypto/hash.hpp>

using namespace std;
using namespace pichi;
using namespace pichi::crypto;
namespace mpl = boost::mpl;

// According to RFC 5869 Appendix A
template <int id> struct RFC5869 {
  static HashAlgorithm const ALGORITHM;
  static vector<uint8_t> const IKM;
  static vector<uint8_t> const SALT;
  static vector<uint8_t> const INFO;
  static size_t const LENGTH;
  static vector<uint8_t> const OKM;
};

template <> HashAlgorithm const RFC5869<1>::ALGORITHM = HashAlgorithm::SHA256;
template <>
vector<uint8_t> const RFC5869<1>::IKM = hex2bin("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
template <> vector<uint8_t> const RFC5869<1>::SALT = hex2bin("000102030405060708090a0b0c");
template <> vector<uint8_t> const RFC5869<1>::INFO = hex2bin("f0f1f2f3f4f5f6f7f8f9");
template <> size_t const RFC5869<1>::LENGTH = 42;
template <>
vector<uint8_t> const RFC5869<1>::OKM =
    hex2bin("3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");

template <> HashAlgorithm const RFC5869<2>::ALGORITHM = HashAlgorithm::SHA256;
template <>
vector<uint8_t> const RFC5869<2>::IKM =
    hex2bin("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a"
            "2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f");
template <>
vector<uint8_t> const RFC5869<2>::SALT =
    hex2bin("606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a"
            "8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf");
template <>
vector<uint8_t> const RFC5869<2>::INFO =
    hex2bin("b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9da"
            "dbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
template <> size_t const RFC5869<2>::LENGTH = 82;
template <>
vector<uint8_t> const RFC5869<2>::OKM =
    hex2bin("b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41"
            "c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87");

template <> HashAlgorithm const RFC5869<3>::ALGORITHM = HashAlgorithm::SHA256;
template <> vector<uint8_t> const RFC5869<3>::IKM = vector<uint8_t>(22, 0x0b);
template <> vector<uint8_t> const RFC5869<3>::SALT = {};
template <> vector<uint8_t> const RFC5869<3>::INFO = {};
template <> size_t const RFC5869<3>::LENGTH = 42;
template <>
vector<uint8_t> const RFC5869<3>::OKM =
    hex2bin("8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8");

template <> HashAlgorithm const RFC5869<4>::ALGORITHM = HashAlgorithm::SHA1;
template <> vector<uint8_t> const RFC5869<4>::IKM = vector<uint8_t>(11, 0x0b);
template <> vector<uint8_t> const RFC5869<4>::SALT = hex2bin("000102030405060708090a0b0c");
template <> vector<uint8_t> const RFC5869<4>::INFO = hex2bin("f0f1f2f3f4f5f6f7f8f9");
template <> size_t const RFC5869<4>::LENGTH = 42;
template <>
vector<uint8_t> const RFC5869<4>::OKM =
    hex2bin("085a01ea1b10f36933068b56efa5ad81a4f14b822f5b091568a9cdd4f155fda2c22e422478d305f3f896");

template <> HashAlgorithm const RFC5869<5>::ALGORITHM = HashAlgorithm::SHA1;
template <>
vector<uint8_t> const RFC5869<5>::IKM =
    hex2bin("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a"
            "2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f");
template <>
vector<uint8_t> const RFC5869<5>::SALT =
    hex2bin("606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a"
            "8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf");
template <>
vector<uint8_t> const RFC5869<5>::INFO =
    hex2bin("b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9da"
            "dbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");
template <> size_t const RFC5869<5>::LENGTH = 82;
template <>
vector<uint8_t> const RFC5869<5>::OKM =
    hex2bin("0bd770a74d1160f7c9f12cd5912a06ebff6adcae899d92191fe4305673ba2ffe8fa3f1a4e5ad79f3f334b3"
            "b202b2173c486ea37ce3d397ed034c7f9dfeb15c5e927336d0441f4c4300e2cff0d0900b52d3b4");

template <> HashAlgorithm const RFC5869<6>::ALGORITHM = HashAlgorithm::SHA1;
template <> vector<uint8_t> const RFC5869<6>::IKM = vector<uint8_t>(22, 0x0b);
template <> vector<uint8_t> const RFC5869<6>::SALT = {};
template <> vector<uint8_t> const RFC5869<6>::INFO = {};
template <> size_t const RFC5869<6>::LENGTH = 42;
template <>
vector<uint8_t> const RFC5869<6>::OKM =
    hex2bin("0ac1af7002b3d761d1e55298da9d0506b9ae52057220a306e07b6b87e8df21d0ea00033de03984d34918");

template <> HashAlgorithm const RFC5869<7>::ALGORITHM = HashAlgorithm::SHA1;
template <> vector<uint8_t> const RFC5869<7>::IKM = vector<uint8_t>(22, 0x0c);
template <> vector<uint8_t> const RFC5869<7>::SALT = vector<uint8_t>(42, 0);
template <> vector<uint8_t> const RFC5869<7>::INFO = {};
template <> size_t const RFC5869<7>::LENGTH = 42;
template <>
vector<uint8_t> const RFC5869<7>::OKM =
    hex2bin("2c91117204d745f3500d636a62f64f0ab3bae548aa53d423b0d1f27ebba6f5e5673a081d70cce7acfc48");

using RFC5869Cases =
    mpl::list<RFC5869<1>, RFC5869<2>, RFC5869<3>, RFC5869<4>, RFC5869<5>, RFC5869<6>, RFC5869<7>>;

template <HashAlgorithm algorithm> struct MaxLenCase {
  static HashAlgorithm const ALGORITHM = algorithm;
  static uint8_t const FRONT;
  static uint8_t const BACK;
  static size_t const LEN;
};

template <> uint8_t const MaxLenCase<HashAlgorithm::MD5>::FRONT = 0xbf;
template <> uint8_t const MaxLenCase<HashAlgorithm::MD5>::BACK = 0xf3;
template <> size_t const MaxLenCase<HashAlgorithm::MD5>::LEN = 4080;

template <> uint8_t const MaxLenCase<HashAlgorithm::SHA1>::FRONT = 0x88;
template <> uint8_t const MaxLenCase<HashAlgorithm::SHA1>::BACK = 0xdc;
template <> size_t const MaxLenCase<HashAlgorithm::SHA1>::LEN = 5100;

template <> uint8_t const MaxLenCase<HashAlgorithm::SHA224>::FRONT = 0xba;
template <> uint8_t const MaxLenCase<HashAlgorithm::SHA224>::BACK = 0x43;
template <> size_t const MaxLenCase<HashAlgorithm::SHA224>::LEN = 7140;

template <> uint8_t const MaxLenCase<HashAlgorithm::SHA256>::FRONT = 0xeb;
template <> uint8_t const MaxLenCase<HashAlgorithm::SHA256>::BACK = 0xce;
template <> size_t const MaxLenCase<HashAlgorithm::SHA256>::LEN = 8160;

template <> uint8_t const MaxLenCase<HashAlgorithm::SHA384>::FRONT = 0x47;
template <> uint8_t const MaxLenCase<HashAlgorithm::SHA384>::BACK = 0xef;
template <> size_t const MaxLenCase<HashAlgorithm::SHA384>::LEN = 12240;

template <> uint8_t const MaxLenCase<HashAlgorithm::SHA512>::FRONT = 0x9d;
template <> uint8_t const MaxLenCase<HashAlgorithm::SHA512>::BACK = 0x7a;
template <> size_t const MaxLenCase<HashAlgorithm::SHA512>::LEN = 16320;

using MaxLenCases = mpl::list<MaxLenCase<HashAlgorithm::MD5>, MaxLenCase<HashAlgorithm::SHA1>,
                              MaxLenCase<HashAlgorithm::SHA224>, MaxLenCase<HashAlgorithm::SHA256>,
                              MaxLenCase<HashAlgorithm::SHA384>, MaxLenCase<HashAlgorithm::SHA512>>;

BOOST_AUTO_TEST_SUITE(HKDF)

BOOST_AUTO_TEST_CASE_TEMPLATE(rfc5869, Case, RFC5869Cases)
{
  auto okm = array<uint8_t, Case::LENGTH>{};
  fill_n(begin(okm), sizeof(okm), 0_u8);
  hkdf<Case::ALGORITHM>(okm, Case::IKM, Case::SALT, Case::INFO);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(Case::OKM), cend(Case::OKM), cbegin(okm), cend(okm));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(maxLen, Case, MaxLenCases)
{
  auto empty = ConstBuffer<uint8_t>{};
  auto okm = array<uint8_t, Case::LEN>{};
  hkdf<Case::ALGORITHM>(okm, empty, empty, empty);
  BOOST_CHECK_EQUAL(Case::FRONT, okm.front());
  BOOST_CHECK_EQUAL(Case::BACK, okm.back());
}

BOOST_AUTO_TEST_SUITE_END()
