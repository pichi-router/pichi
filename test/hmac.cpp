#define BOOST_TEST_MODULE pichi hmac test
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 50

#include "utils.hpp"
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/crypto/hash.hpp>
#include <vector>

using namespace std;
namespace mpl = boost::mpl;

namespace pichi::unit_test {

using namespace crypto;

template <HashAlgorithm algorithm, int id> struct TestCase {
  using HmacType = Hmac<algorithm>;
  static vector<uint8_t> const KEY;
  static vector<uint8_t> const DATA;
  static vector<uint8_t> const DIGEST;
};

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 0>::DATA = {};
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 0>::DIGEST = hex2bin("74e6f7298a9c2d168935f58c001bad88");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 0>::DATA = {};
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 0>::DIGEST = hex2bin("fbdb1d1b18aa6c08324b7d64b71fb76370690e1d");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 0>::DATA = {};
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 0>::DIGEST =
    hex2bin("5ce14f72894662213e2748d2a6ba234b74263910cedde2f5a9271524");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 0>::DATA = {};
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 0>::DIGEST =
    hex2bin("b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 0>::DATA = {};
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 0>::DIGEST =
    hex2bin("6c1f2ee938fad2e24bd91298474382ca218c75db3d83e114b3d4367776d14d3551289e75e8209cd4b79230"
            "2840234adc");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 0>::KEY = {};
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 0>::DATA = {};
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 0>::DIGEST =
    hex2bin("b936cee86c9f87aa5d3c6f2e84cb5a4239a5fe50480a6ec66b70ab5b1f4ac6730c6c515421b327ec1d6940"
            "2e53dfb49ad7381eb067b338fd7b0cb22247225d47");

// MD5 cases acoording to RFC 2202 A.2
template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 1>::KEY = vector<uint8_t>(16, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 1>::DIGEST = hex2bin("9294727a3638bb1c13f48ef8158bfc9d");

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 2>::DIGEST = hex2bin("750c783e6ab0b503eaa86e310a5db738");

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 3>::KEY = vector<uint8_t>(16, 0xaa);
template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 3>::DIGEST = hex2bin("56be34521d144c88dbb8c733f0e8b3f6");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::MD5, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 4>::DIGEST = hex2bin("697eaf0aca3a3aea3a75164746ffaa79");

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 5>::KEY = vector<uint8_t>(16, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::MD5, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 5>::DIGEST = hex2bin("56461ef2342edc00f9bab995690efd4c");

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 6>::KEY = vector<uint8_t>(80, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::MD5, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 6>::DIGEST = hex2bin("6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd");

template <> vector<uint8_t> const TestCase<HashAlgorithm::MD5, 7>::KEY = vector<uint8_t>(80, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::MD5, 7>::DATA =
    str2vec("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::MD5, 7>::DIGEST = hex2bin("6f630fad67cda0ee1fb1f562db3aa53e");

// SHA1 cases acoording to RFC 2202 A.2
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 1>::KEY = vector<uint8_t>(20, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 1>::DIGEST = hex2bin("b617318655057264e28bc0b6fb378c8ef146be00");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 2>::DIGEST = hex2bin("effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 3>::KEY = vector<uint8_t>(20, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 3>::DIGEST = hex2bin("125d7342b9ac11cd91a39af48aa17b4f63f175d3");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 4>::DIGEST = hex2bin("4c9007f4026250c6bc8414f9bf50c86c2d7235da");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 5>::KEY = vector<uint8_t>(20, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 5>::DIGEST = hex2bin("4c1a03424b55e07fe7f27be1d58bb9324a9a5a04");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 6>::KEY = vector<uint8_t>(80, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 6>::DIGEST = hex2bin("aa4ae5e15272d00e95705637ce8a3b55ed402112");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 7>::KEY = vector<uint8_t>(80, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA1, 7>::DATA =
    str2vec("Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA1, 7>::DIGEST = hex2bin("e8e99d0f45237d786d6bbaa7965c7808bbff1a91");

// SHA224 cases acoording to RFC 4231 A.4
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 1>::KEY = vector<uint8_t>(20, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 1>::DIGEST =
    hex2bin("896fb1128abbdf196832107cd49df33f47b4b1169912ba4f53684b22");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA224, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 2>::DIGEST =
    hex2bin("a30e01098bc6dbbf45690f3a7e9e6d0f8bbea2a39e6148008fd05e44");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 3>::KEY = vector<uint8_t>(20, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 3>::DIGEST =
    hex2bin("7fb3cb3588c6c1f6ffa9694d7d6ad2649365b0c1f65d69d1ec8333ea");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 4>::DIGEST =
    hex2bin("6c11506874013cac6a2abc1bb382627cec6a90d86efc012de7afec5a");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 5>::KEY = vector<uint8_t>(20, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA224, 5>::DIGEST = hex2bin("0e2aea68a90c8d37c988bcdb9fca6fa8");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 6>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 6>::DIGEST =
    hex2bin("95e9a0db962095adaebe9b2d6f0dbce2d499f112f2d2b7273fa6870e");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 7>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 7>::DATA =
    str2vec("This is a test using a larger than block-size key and a larger than block-size data. "
            "The key needs to be hashed before being used by the HMAC algorithm.");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA224, 7>::DIGEST =
    hex2bin("3a854166ac5d9f023f54d517d0b39dbd946770db9c2b95c9f6f565d1");

// SHA256 cases acoording to RFC 4231 A.4
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 1>::KEY = vector<uint8_t>(20, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 1>::DIGEST =
    hex2bin("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA256, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 2>::DIGEST =
    hex2bin("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 3>::KEY = vector<uint8_t>(20, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 3>::DIGEST =
    hex2bin("773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 4>::DIGEST =
    hex2bin("82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 5>::KEY = vector<uint8_t>(20, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA256, 5>::DIGEST = hex2bin("a3b6167473100ee06e0c796c2955552b");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 6>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 6>::DIGEST =
    hex2bin("60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 7>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 7>::DATA =
    str2vec("This is a test using a larger than block-size key and a larger than block-size data. "
            "The key needs to be hashed before being used by the HMAC algorithm.");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA256, 7>::DIGEST =
    hex2bin("9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2");

// SHA384 cases acoording to RFC 4231 A.4
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 1>::KEY = vector<uint8_t>(20, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 1>::DIGEST =
    hex2bin("afd03944d84895626b0825f4ab46907f15f9dadbe4101ec682aa034c7cebc59cfaea9ea9076ede7f4af152"
            "e8b2fa9cb6");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA384, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 2>::DIGEST =
    hex2bin("af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e8e2240ca5e69e2c78b3239"
            "ecfab21649");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 3>::KEY = vector<uint8_t>(20, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 3>::DIGEST =
    hex2bin("88062608d3e6ad8a0aa2ace014c8a86f0aa635d947ac9febe83ef4e55966144b2a5ab39dc13814b94e3ab6"
            "e101a34f27");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 4>::DIGEST =
    hex2bin("3e8a69b7783c25851933ab6290af6ca77a9981480850009cc5577c6e1f573b4e6801dd23c4a7d679ccf8a3"
            "86c674cffb");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 5>::KEY = vector<uint8_t>(20, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA384, 5>::DIGEST = hex2bin("3abf34c3503b2a23a46efc619baef897");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 6>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 6>::DIGEST =
    hex2bin("4ece084485813e9088d2c63a041bc5b44f9ef1012a2b588f3cd11f05033ac4c60c2ef6ab4030fe8296248d"
            "f163f44952");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 7>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 7>::DATA =
    str2vec("This is a test using a larger than block-size key and a larger than block-size data. "
            "The key needs to be hashed before being used by the HMAC algorithm.");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA384, 7>::DIGEST =
    hex2bin("6617178e941f020d351e2f254e8fd32c602420feb0b8fb9adccebb82461e99c5a678cc31e799176d3860e6"
            "110c46523e");

// SHA512 cases acoording to RFC 4231 A.4
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 1>::KEY = vector<uint8_t>(20, 0x0b);
template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 1>::DATA = str2vec("Hi There");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 1>::DIGEST =
    hex2bin("87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b27"
            "4eaea3f4e4be9d914eeb61f1702e696c203a126854");

template <> vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 2>::KEY = str2vec("Jefe");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA512, 2>::DATA = str2vec("what do ya want for nothing?");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 2>::DIGEST =
    hex2bin("164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f"
            "65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 3>::KEY = vector<uint8_t>(20, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 3>::DATA = vector<uint8_t>(50, 0xdd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 3>::DIGEST =
    hex2bin("fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39bf3e848279a722c806b485"
            "a47e67c807b946a337bee8942674278859e13292fb");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 4>::KEY =
    hex2bin("0102030405060708090a0b0c0d0e0f10111213141516171819");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 4>::DATA = vector<uint8_t>(50, 0xcd);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 4>::DIGEST =
    hex2bin("b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3dba91ca5c11aa25eb4d67927"
            "5cc5788063a5f19741120c4f2de2adebeb10a298dd");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 5>::KEY = vector<uint8_t>(20, 0x0c);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 5>::DATA = str2vec("Test With Truncation");
template <>
vector<uint8_t> const
    TestCase<HashAlgorithm::SHA512, 5>::DIGEST = hex2bin("415fad6271580a531d4179bc891d87a6");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 6>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 6>::DATA =
    str2vec("Test Using Larger Than Block-Size Key - Hash Key First");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 6>::DIGEST =
    hex2bin("80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2"
            "215d6a1e5295e64f73f63f0aec8b915a985d786598");

template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 7>::KEY = vector<uint8_t>(131, 0xaa);
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 7>::DATA =
    str2vec("This is a test using a larger than block-size key and a larger than block-size data. "
            "The key needs to be hashed before being used by the HMAC algorithm.");
template <>
vector<uint8_t> const TestCase<HashAlgorithm::SHA512, 7>::DIGEST =
    hex2bin("e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944b6022cac3c4982b10d5eeb"
            "55c3e4de15134676fb6de0446065c97440fa8c6a58");

using TestCases = mpl::list<TestCase<HashAlgorithm::MD5, 0>, TestCase<HashAlgorithm::MD5, 1>,
                            TestCase<HashAlgorithm::MD5, 2>, TestCase<HashAlgorithm::MD5, 3>,
                            TestCase<HashAlgorithm::MD5, 4>, TestCase<HashAlgorithm::MD5, 5>,
                            TestCase<HashAlgorithm::MD5, 6>, TestCase<HashAlgorithm::MD5, 7>,
                            TestCase<HashAlgorithm::SHA1, 0>, TestCase<HashAlgorithm::SHA1, 1>,
                            TestCase<HashAlgorithm::SHA1, 2>, TestCase<HashAlgorithm::SHA1, 3>,
                            TestCase<HashAlgorithm::SHA1, 4>, TestCase<HashAlgorithm::SHA1, 5>,
                            TestCase<HashAlgorithm::SHA1, 6>, TestCase<HashAlgorithm::SHA1, 7>,
                            TestCase<HashAlgorithm::SHA224, 0>, TestCase<HashAlgorithm::SHA224, 1>,
                            TestCase<HashAlgorithm::SHA224, 2>, TestCase<HashAlgorithm::SHA224, 3>,
                            TestCase<HashAlgorithm::SHA224, 4>, TestCase<HashAlgorithm::SHA224, 5>,
                            TestCase<HashAlgorithm::SHA224, 6>, TestCase<HashAlgorithm::SHA224, 7>,
                            TestCase<HashAlgorithm::SHA256, 0>, TestCase<HashAlgorithm::SHA256, 1>,
                            TestCase<HashAlgorithm::SHA256, 2>, TestCase<HashAlgorithm::SHA256, 3>,
                            TestCase<HashAlgorithm::SHA256, 4>, TestCase<HashAlgorithm::SHA256, 5>,
                            TestCase<HashAlgorithm::SHA256, 6>, TestCase<HashAlgorithm::SHA256, 7>,
                            TestCase<HashAlgorithm::SHA384, 0>, TestCase<HashAlgorithm::SHA384, 1>,
                            TestCase<HashAlgorithm::SHA384, 2>, TestCase<HashAlgorithm::SHA384, 3>,
                            TestCase<HashAlgorithm::SHA384, 4>, TestCase<HashAlgorithm::SHA384, 5>,
                            TestCase<HashAlgorithm::SHA384, 6>, TestCase<HashAlgorithm::SHA384, 7>,
                            TestCase<HashAlgorithm::SHA512, 0>, TestCase<HashAlgorithm::SHA512, 1>,
                            TestCase<HashAlgorithm::SHA512, 2>, TestCase<HashAlgorithm::SHA512, 3>,
                            TestCase<HashAlgorithm::SHA512, 4>, TestCase<HashAlgorithm::SHA512, 5>,
                            TestCase<HashAlgorithm::SHA512, 6>, TestCase<HashAlgorithm::SHA512, 7>>;

BOOST_AUTO_TEST_SUITE(HMAC)

BOOST_AUTO_TEST_CASE_TEMPLATE(Hmac_Cases, Case, TestCases)
{
  using HMAC = typename Case::HmacType;
  auto container = array<uint8_t, 128>{};
  auto fact = MutableBuffer{container, Case::DIGEST.size()};

  fill_n(begin(fact), fact.size(), 0_u8);
  HMAC{Case::KEY}.hash(Case::DATA, fact);

  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(Case::DIGEST), cend(Case::DIGEST), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
