#define BOOST_TEST_MODULE pichi key test

#include "utils.hpp"
#include <algorithm>
#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <pichi/crypto/key.hpp>
#include <pichi/crypto/method.hpp>
#include <utility>
#include <vector>

using namespace std;
namespace mpl = boost::mpl;

namespace pichi::unit_test {

using namespace crypto;

static auto const cases = vector<pair<vector<uint8_t>, vector<uint8_t>>>{
    make_pair(vector<uint8_t>{},
              hex2bin("d41d8cd98f00b204e9800998ecf8427e59adb24ef3cdbe0297f05b395827453f")),
    make_pair(str2vec("Hi There"),
              hex2bin("5b49b515f3173e4540b7d39bb57a4482f1b1d0d6dbb76cc6a54c6432fc7d361c")),
    make_pair(str2vec("what do ya want for nothing?"),
              hex2bin("d03cb659cbf9192dcd066272249f841235f9a69f9840003edb22e6edd60543cf")),
    make_pair(vector<uint8_t>(50, 0xdd),
              hex2bin("b3af4940b3b7a0e7448cbfbb6ab04cc8a2faf9a0491cbbc4640315166074c17c")),
    make_pair(vector<uint8_t>(50, 0xcd),
              hex2bin("999732b72ceff665b3f7608411db66a4ff7072fd61273c9a6b14a27091a5cea8")),
    make_pair(str2vec("Test With Truncation"),
              hex2bin("dbcc9d8a88e5287213bc3556f8f8a4987d38b56b1b662007ed68265a574b7637"))};

template <CryptoMethod method, size_t size> struct MHelper {
  static auto const METHOD = method;
  static auto const SIZE = size;
};

using Helpers = mpl::list<
    MHelper<CryptoMethod::RC4_MD5, 16>, MHelper<CryptoMethod::BF_CFB, 16>,
    MHelper<CryptoMethod::AES_128_CTR, 16>, MHelper<CryptoMethod::AES_192_CTR, 24>,
    MHelper<CryptoMethod::AES_256_CTR, 32>, MHelper<CryptoMethod::AES_128_CFB, 16>,
    MHelper<CryptoMethod::AES_192_CFB, 24>, MHelper<CryptoMethod::AES_256_CFB, 32>,
    MHelper<CryptoMethod::CAMELLIA_128_CFB, 16>, MHelper<CryptoMethod::CAMELLIA_192_CFB, 24>,
    MHelper<CryptoMethod::CAMELLIA_256_CFB, 32>, MHelper<CryptoMethod::CHACHA20, 32>,
    MHelper<CryptoMethod::SALSA20, 32>, MHelper<CryptoMethod::CHACHA20_IETF, 32>,
    MHelper<CryptoMethod::AES_128_GCM, 16>, MHelper<CryptoMethod::AES_192_GCM, 24>,
    MHelper<CryptoMethod::AES_256_GCM, 32>, MHelper<CryptoMethod::CHACHA20_IETF_POLY1305, 32>,
    MHelper<CryptoMethod::XCHACHA20_IETF_POLY1305, 32>>;

BOOST_AUTO_TEST_SUITE(KEYS)

BOOST_AUTO_TEST_CASE_TEMPLATE(generateKey_Normal, Helper, Helpers)
{
  for_each(cbegin(cases), cend(cases), [](auto const& pair) {
    auto container = array<uint8_t, 1024>{0};
    auto pw = ConstBuffer<uint8_t>{pair.first};
    auto expt = ConstBuffer<uint8_t>{pair.second, Helper::SIZE};
    auto fact = MutableBuffer<uint8_t>{container, Helper::SIZE};
    BOOST_CHECK(generateKey(Helper::METHOD, pw, fact) == Helper::SIZE);
    BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
  });
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
