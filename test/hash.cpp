#define BOOST_TEST_MODULE pichi hash test

#include "utils.hpp"
#include <array>
#include <boost/test/unit_test.hpp>
#include <pichi/common/buffer.hpp>
#include <pichi/crypto/hash.hpp>

using namespace std;

namespace pichi::unit_test {

using namespace crypto;

template <HashAlgorithm algorithm> void verify(string_view hex, ConstBuffer<uint8_t> raw = {})
{
  auto expt = hex2bin(hex);
  auto fact = array<uint8_t, HashTraits<algorithm>::length>{0};
  pichi::crypto::Hash<algorithm>{}.hash(raw, fact);
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expt), cend(expt), cbegin(fact), cend(fact));
}

template <HashAlgorithm algorithm> void verify(string_view hex, string_view raw)
{
  verify<algorithm>(hex,
                    ConstBuffer<uint8_t>{reinterpret_cast<uint8_t const*>(raw.data()), raw.size()});
}

BOOST_AUTO_TEST_SUITE(Hash)

BOOST_AUTO_TEST_CASE(Empty)
{
  verify<HashAlgorithm::MD5>("d41d8cd98f00b204e9800998ecf8427e");
  verify<HashAlgorithm::SHA1>("da39a3ee5e6b4b0d3255bfef95601890afd80709");
  verify<HashAlgorithm::SHA224>("d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f");
  verify<HashAlgorithm::SHA256>("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  verify<HashAlgorithm::SHA384>("38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da27"
                                "4edebfe76f65fbd51ad2f14898b95b");
  verify<HashAlgorithm::SHA512>("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47"
                                "d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
}

// According to RFC 1321 A.5
BOOST_AUTO_TEST_CASE(MD5)
{
  verify<HashAlgorithm::MD5>("d41d8cd98f00b204e9800998ecf8427e", "");
  verify<HashAlgorithm::MD5>("0cc175b9c0f1b6a831c399e269772661", "a");
  verify<HashAlgorithm::MD5>("900150983cd24fb0d6963f7d28e17f72", "abc");
  verify<HashAlgorithm::MD5>("f96b697d7cb7938d525a2f31aaf161d0", "message digest");
  verify<HashAlgorithm::MD5>("c3fcd3d76192e4007dfb496cca67e13b", "abcdefghijklmnopqrstuvwxyz");
  verify<HashAlgorithm::MD5>("d174ab98d277d9f5a5611c2c9f419d9f",
                             "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
  verify<HashAlgorithm::MD5>(
      "57edf4a22be3c955ac49da2e2107b67a",
      "12345678901234567890123456789012345678901234567890123456789012345678901234567890");
}

// According to https://www.di-mgt.com.au/sha_testvectors.html
BOOST_AUTO_TEST_CASE(SHA1)
{
  verify<HashAlgorithm::SHA1>("da39a3ee5e6b4b0d3255bfef95601890afd80709", "");
  verify<HashAlgorithm::SHA1>("a9993e364706816aba3e25717850c26c9cd0d89d", "abc");
  verify<HashAlgorithm::SHA1>("84983e441c3bd26ebaae4aa1f95129e5e54670f1",
                              "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  verify<HashAlgorithm::SHA1>("a49b2446a02c645bf419f995b67091253a04a259",
                              "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijkl"
                              "mnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  verify<HashAlgorithm::SHA1>("34aa973cd4c4daa4f61eeb2bdbad27316534016f",
                              vector<uint8_t>(1000000, 'a'));
}

// According to https://www.di-mgt.com.au/sha_testvectors.html
BOOST_AUTO_TEST_CASE(SHA224)
{
  verify<HashAlgorithm::SHA224>("d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f", "");
  verify<HashAlgorithm::SHA224>("23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7", "abc");
  verify<HashAlgorithm::SHA224>("75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525",
                                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  verify<HashAlgorithm::SHA224>("c97ca9a559850ce97a04a96def6d99a9e0e0e2ab14e6b8df265fc0b3",
                                "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoij"
                                "klmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  verify<HashAlgorithm::SHA224>("20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67",
                                vector<uint8_t>(1000000, 'a'));
}

// According to https://www.di-mgt.com.au/sha_testvectors.html
BOOST_AUTO_TEST_CASE(SHA256)
{
  verify<HashAlgorithm::SHA256>("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                                "");
  verify<HashAlgorithm::SHA256>("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                                "abc");
  verify<HashAlgorithm::SHA256>("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
                                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  verify<HashAlgorithm::SHA256>("cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1",
                                "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoij"
                                "klmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  verify<HashAlgorithm::SHA256>("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
                                vector<uint8_t>(1000000, 'a'));
}

// According to https://www.di-mgt.com.au/sha_testvectors.html
BOOST_AUTO_TEST_CASE(SHA384)
{
  verify<HashAlgorithm::SHA384>("38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da27"
                                "4edebfe76f65fbd51ad2f14898b95b",
                                "");
  verify<HashAlgorithm::SHA384>("cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed80"
                                "86072ba1e7cc2358baeca134c825a7",
                                "abc");
  verify<HashAlgorithm::SHA384>("3391fdddfc8dc7393707a65b1b4709397cf8b1d162af05abfe8f450de5f36bc6b0"
                                "455a8520bc4e6f5fe95b1fe3c8452b",
                                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  verify<HashAlgorithm::SHA384>("09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fc"
                                "c7c71a557e2db966c3e9fa91746039",
                                "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoij"
                                "klmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  verify<HashAlgorithm::SHA384>("9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b07"
                                "b8b3dc38ecc4ebae97ddd87f3d8985",
                                vector<uint8_t>(1000000, 'a'));
}

// According to https://www.di-mgt.com.au/sha_testvectors.html
BOOST_AUTO_TEST_CASE(SHA512)
{
  verify<HashAlgorithm::SHA512>("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47"
                                "d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
                                "");
  verify<HashAlgorithm::SHA512>("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a21"
                                "92992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
                                "abc");
  verify<HashAlgorithm::SHA512>("204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596"
                                "fd15c13b1b07f9aa1d3bea57789ca031ad85c7a71dd70354ec631238ca3445",
                                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
  verify<HashAlgorithm::SHA512>("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb688901850"
                                "1d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909",
                                "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoij"
                                "klmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
  verify<HashAlgorithm::SHA512>("e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde"
                                "0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b",
                                vector<uint8_t>(1000000, 'a'));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
