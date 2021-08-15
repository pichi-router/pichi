#define BOOST_TEST_MODULE pichi stream test

#include "utils.hpp"
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <numeric>
#include <pichi/crypto/aead.hpp>
#include <pichi/crypto/method.hpp>
#include <pichi/crypto/stream.hpp>

using namespace std;
namespace mpl = boost::mpl;

namespace pichi::unit_test {

using namespace crypto;

template <size_t> struct KeyIv {
  static vector<uint8_t> const KEY;
  static vector<uint8_t> const IV;
};

template <> vector<uint8_t> const KeyIv<16>::KEY = hex2bin("2b7e151628aed2a6abf7158809cf4f3c");
template <>
vector<uint8_t> const KeyIv<24>::KEY = hex2bin("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
template <>
vector<uint8_t> const
    KeyIv<32>::KEY = hex2bin("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");

template <> vector<uint8_t> const KeyIv<8>::IV = hex2bin("0123456789abcdef");
template <> vector<uint8_t> const KeyIv<12>::IV = hex2bin("0123456789abcdef01234567");
template <> vector<uint8_t> const KeyIv<16>::IV = hex2bin("000102030405060708090a0b0c0d0e0f");
template <>
vector<uint8_t> const KeyIv<24>::IV = hex2bin("0123456789abcdef000102030405060708090a0b0c0d0e0f");
template <>
vector<uint8_t> const
    KeyIv<32>::IV = hex2bin("000102030405060708090a0b0c0d0e0ff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff");

static auto const plains = vector<vector<uint8_t>>{
    str2vec("Any submission t"),  str2vec("o the IETF inten"),  str2vec("ded by the Contr"),
    str2vec("ibutor for publi"),  str2vec("cation as all or"),  str2vec(" part of an IETF"),
    str2vec(" Internet-Draft "),  str2vec("or RFC and any s"),  str2vec("tatement made wi"),
    str2vec("thin the context"),  str2vec(" of an IETF acti"),  str2vec("vity is consider"),
    str2vec("ed an \"IETF Cont"), str2vec("ribution\". Such "), str2vec("statements inclu"),
    str2vec("de oral statemen"),  str2vec("ts in IETF sessi"),  str2vec("ons, as well as "),
    str2vec("written and elec"),  str2vec("tronic communica"),  str2vec("tions made at an"),
    str2vec("y time or place,"),  str2vec(" which are addre"),  str2vec("ssed to")};

template <CryptoMethod method> struct Ciphers {
  static CryptoMethod const METHOD = method;
  static vector<vector<uint8_t>> const CIPHERS;
  static vector<uint8_t> const& KEY;
  static vector<uint8_t> const& IV;
};

#if MBEDTLS_VERSION_MAJOR < 3
template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::RC4_MD5>::CIPHERS = {
    hex2bin("cd1695a36fbfb5270d59fcf7153e7dcf"), hex2bin("eca24c0e95c67b736c52e75d66355385"),
    hex2bin("640ce36b2a138679e54ce1c3994a368c"), hex2bin("a50e69616069e56343974bb4b491c47f"),
    hex2bin("5bce533f5344178c7d41f44d5aa04318"), hex2bin("1b446cd062ba6e848669c1ffae5503d7"),
    hex2bin("123795db0f254718f0660ae851e3c4c3"), hex2bin("34b117f2f0dd251a940eb5f0d1323cde"),
    hex2bin("4fadd407b3055cbd8f979cf15dbce9bb"), hex2bin("effff351d8608f3a70a1e14bf64274e9"),
    hex2bin("50a1072690c6765d41e5e815101be85b"), hex2bin("e02ba7bd989ebd2e9cabc28d8c96b0e3"),
    hex2bin("0153aad1543f7508bda77c5d0fab1f91"), hex2bin("d0a0687d5b1472d2367b4555b5b38684"),
    hex2bin("e2870e3df03b5335184b2d5a01f8e7a9"), hex2bin("89765f312f10a4b499c3da0c7bbbc050"),
    hex2bin("346194e328dcbf116e939c0b73362d23"), hex2bin("c3b81b65d178dcc4f0cfd6e0179a4a4f"),
    hex2bin("d6ccf108f9192f8cebdc2ef06983c8f3"), hex2bin("dff2cf1adbd0a99ee332195abaefa887"),
    hex2bin("c49856bb799ae40f20e2d4a3a4fed641"), hex2bin("18e6d70f2dc7d82a39aaa4ea3da63a9a"),
    hex2bin("a1239dd8fc3eb37305e6a9f63d83ad18"), hex2bin("c33e6a8ab03515")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::RC4_MD5>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::RC4_MD5>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::BF_CFB>::CIPHERS = {
    hex2bin("b19c0b80b42318e7e4141a5cb430c152"), hex2bin("6ac82994324f04a71a2e4a6fe93bafb9"),
    hex2bin("b413e3320fd6f5ca88b69e44f3b9e353"), hex2bin("3c0929cc878f797cd6c95653af307e43"),
    hex2bin("948a6bb3544dc9e539758430874b93bd"), hex2bin("40ae6c7489a62bea1014c50b872ea72a"),
    hex2bin("3aeffaf47e866acc6df7681f45fda01b"), hex2bin("38223f511cf8b99b92867dc8f1d221ae"),
    hex2bin("850294c276dd41b74e4b09c1e51fc5af"), hex2bin("99b2f33d0f8f91e6b476b2642dc5029e"),
    hex2bin("cabac16def2bec55f503f9c3e6464d55"), hex2bin("c8e81c484dc180c6a231b65d48256de1"),
    hex2bin("d5bf92b31b9f242664584dbcd941a4b7"), hex2bin("e1c31af2a324719cc5e1ed54d20323da"),
    hex2bin("7306ff46d831eb933fa09e37afad94d3"), hex2bin("3667e04524f21ca1153aee7085e585d5"),
    hex2bin("8c750f9bd10a2e09bc129bb613b81930"), hex2bin("cefb9cd2453ade0a900a0917736e3717"),
    hex2bin("b75e5a2861d78c780800ce81ee864724"), hex2bin("242905d5dd53d454b24076e3e4c98ce2"),
    hex2bin("28019ff750e43c5c3b81025259e7bec1"), hex2bin("2bb35030a27f25bb1ce51f96ca2c9992"),
    hex2bin("985445eed87575ffd8f105c70fcc6db9"), hex2bin("cd8fd745369f58")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::BF_CFB>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::BF_CFB>::IV = KeyIv<8>::IV;
#endif  // MBEDTLS_VERSION_MAJOR < 3

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_128_CTR>::CIPHERS = {
    hex2bin("11901eecea1850dbb37a4480f4c1cc14"), hex2bin("c001fa26e2c3daef8e776b8d1583c631"),
    hex2bin("f28feba967fab036d1aba0db63e35242"), hex2bin("b2a8f476b61a872115ed99a35ed235ae"),
    hex2bin("7bb254b73739d66c9de39f612b7e531b"), hex2bin("8c3e49a28601b6e9c849b794c45cc990"),
    hex2bin("1c6566b6c500297662929f22fc3e1325"), hex2bin("13d0b6ac21472137c5a4f14c416d8d42"),
    hex2bin("624b43471978bab743b0783d3038c05f"), hex2bin("1fae731062d9e3ce5db3e4fea06c36c5"),
    hex2bin("db724366637cf1b6c016e6e02d242cc1"), hex2bin("1ea534daedc417f8db9239635681d53d"),
    hex2bin("bdd3f29095547f8abd2eca2512044913"), hex2bin("9bcf9a7afa386c5ed3a49899d04f5c22"),
    hex2bin("44ef6381e3d280f88de6ed4ac899d7cf"), hex2bin("db92cdcae7a3d2491e9e8a167a34de04"),
    hex2bin("a7f1c689985d7036e158d9b35765e28a"), hex2bin("862024d0f1c712e057425152bcd48bae"),
    hex2bin("fe536e515206a00de9220044f43518d1"), hex2bin("9dd333822d61c183e7376875c03da1db"),
    hex2bin("bbfaa6749da25359373ca5a1958085f8"), hex2bin("06f9622b9f6011e4f9d82a9584543ed8"),
    hex2bin("11f7a6c4ea87cc3d1f75077caa6313cf"), hex2bin("49a6fed10467b1")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_CTR>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_CTR>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_192_CTR>::CIPHERS = {
    hex2bin("e767caad80c47150b48c5471d567762a"), hex2bin("f6d558066c3c3913a91a9eb9c90cd925"),
    hex2bin("2d0db10d92e22a895473900db120dae7"), hex2bin("1ba16683452cd20accd0b4e6f3ca8fe7"),
    hex2bin("5feacffe958850f9f0384c569383fbcf"), hex2bin("44482896295c14d54b902e9ebf4f340b"),
    hex2bin("283f06cb3f788c2c1d93a86077e6ff88"), hex2bin("a826548228acacf7d0838c61a7f2997f"),
    hex2bin("eda59c4764352006736a71b79aaeebe6"), hex2bin("7be9fcd5e8397d0a82387417ab654b61"),
    hex2bin("7c7444d9ae9d5489db191b7ff5b07e87"), hex2bin("233f3912fbacfcef9e560f74f593be40"),
    hex2bin("dc2e2963f6aeff2e2d9b8ee70d2093ba"), hex2bin("0c484de5af7c5025d81740c3eb3a6bc2"),
    hex2bin("ef86df074b7ff2d95901d07194cf2817"), hex2bin("e3340a85c2b84a039c4615f9b20030be"),
    hex2bin("01889f8c794117744b74910b2a901c96"), hex2bin("2294ef534f5b2da703000ab51a19acd2"),
    hex2bin("f4d14fdb863ad16825a93087a3eaa90f"), hex2bin("2368779093cefc4792150fe61f808e33"),
    hex2bin("013acbd6d15ae176af50146a9461d467"), hex2bin("09c4d039cafbbf790b603ed935f76033"),
    hex2bin("05d3b197ff7144542b5a860293743101"), hex2bin("1d080d188251e1")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_CTR>::KEY = KeyIv<24>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_CTR>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_256_CTR>::CIPHERS = {
    hex2bin("f6d1437d874cebb0fe8389fe84a00f3e"), hex2bin("147a86b4f6a960ad9fe2ea102459f40f"),
    hex2bin("ab44153a346ee7e8411386d9dd1b1d77"), hex2bin("76520552f4f627d513bc32f453659c47"),
    hex2bin("aeaf87f069f24590b80c201fc5ab8ef3"), hex2bin("161e282706b1c2b1b6913905157f64af"),
    hex2bin("f28ef48e362f8d41e2d7234d8b0981b2"), hex2bin("3fdf6e2d5448cf271943c4a67ae5fd2f"),
    hex2bin("b2c549086a3232f90e033a6a8fdc4728"), hex2bin("29b4f6145139035c48a88c7e18aae95d"),
    hex2bin("22c0cdcc7c6a4ba19cb961e48d3d93ba"), hex2bin("897cc0bf33f179a49e2de3da6d4d39d3"),
    hex2bin("b13da8cf4db1a9b1617f8e398a03fe24"), hex2bin("77576e82bf49eedc695e6635ce00bef7"),
    hex2bin("38b2df48756fa4be41e4852bc1600cd4"), hex2bin("90e4ec1d5f01bed01d50929381f305cd"),
    hex2bin("28ff6efd25823562100fba9c4f1b15cd"), hex2bin("8b593877b2427a0f98fc49d934b1c125"),
    hex2bin("c50678f4c4580b9aefbadb73463d3593"), hex2bin("eba62addfd7013e198376619306da137"),
    hex2bin("fe26b7bc8d64e2e0210df29a98c150a4"), hex2bin("832850495a9c7c4d9d47357b5a2002f0"),
    hex2bin("75da3a5f1de0089313557c8cd112aa19"), hex2bin("bf282d2030865c")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_CTR>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_CTR>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_128_CFB>::CIPHERS = {
    hex2bin("11901eecea1850dbb37a4480f4c1cc14"), hex2bin("c48df5efeffee2b2af642b6817572685"),
    hex2bin("4f0981b7ac5315dc6c099d10fb543f01"), hex2bin("eac27587a5c1e9b0a3b177f7532ec21b"),
    hex2bin("d4f134b61b592a1316de603309a7947f"), hex2bin("97882f17ee8470a4492dc52801bff4fb"),
    hex2bin("7b72b723ee4b7ea1fc91ebc1cbdd89d2"), hex2bin("0c5e8191ebd2cc8d73267d12b8966cd1"),
    hex2bin("16866884f3ea5a311d58d9e8b6fd352c"), hex2bin("0e0c7c12a468dfe7f8cac5ba23786337"),
    hex2bin("afc6308b5c3dc85b875bd73a596819b7"), hex2bin("2c974b55c792764e592a1dd5a5d0553a"),
    hex2bin("aa0bf0d3b75f2e892324954fe59862b6"), hex2bin("8f93b9ec965f3ab9df2000cb7f4b128a"),
    hex2bin("3c195e6320fda31370d0cf8aba271f52"), hex2bin("a150f6a4fb36973ae1ba2f7dcd5dde76"),
    hex2bin("1eac9e81eaff034161109ad662d710b6"), hex2bin("42288e41bf40c38a4c8c0db79bd243f6"),
    hex2bin("9a7e398b05760e7fef1a5a2b57c6cc30"), hex2bin("c5b28342db5a52ea482495e087d03b85"),
    hex2bin("f966821dee9dafc196d8c07e1064920a"), hex2bin("df1e9890771a495b2a858e4bfb98da8d"),
    hex2bin("fb4b022c642c21fe4b502a891c50ab18"), hex2bin("cb9542a6e3c6f1")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_CFB>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_192_CFB>::CIPHERS = {
    hex2bin("e767caad80c47150b48c5471d567762a"), hex2bin("6f49c5e49416bfa439dba5f49344978a"),
    hex2bin("3a7ae7e077573e30c84d9b0d47b9205a"), hex2bin("e1d71f83bbf3f671d8e52dda2be290b5"),
    hex2bin("2363249d7163d9d3bcacd7b59027c25c"), hex2bin("cedb0d92cf3fe009ae4f96be23a7a5f3"),
    hex2bin("2fd632119314a305b385fe1a6ad23e9e"), hex2bin("b5fdacc8972153e58d7234837ce5bbcc"),
    hex2bin("fc9602de4315af488befc963402f7169"), hex2bin("cf9f41034dccafa848160b71da0f9abe"),
    hex2bin("6f163e116ddee20a7b8433f5075c2dec"), hex2bin("00423a2d2f5442189a4de23d146ce218"),
    hex2bin("fa902605999d985e9029dd25103303cf"), hex2bin("1489142a677b889cffb4ce74427aac06"),
    hex2bin("d02c124fc37aa8ce15d9c2d53f0c31f6"), hex2bin("fa1307421bd663adfa708b4d3a0133c1"),
    hex2bin("c9d6c61cea6f5c1e44e64940bb1bd6d6"), hex2bin("cb28a5d3be0b50ed88b603e71856f29b"),
    hex2bin("13273b19354d7469dee2dfcf3a1e6622"), hex2bin("3fe51b88261f46b9e2f8421df1f09121"),
    hex2bin("efe511ad1fc0a97749c5e5e52c94de5e"), hex2bin("b95f2f46372276b71b981d204456a2ef"),
    hex2bin("4e5435741d9aa78b8e690f93c1415848"), hex2bin("940859e90ada8c")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_CFB>::KEY = KeyIv<24>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_256_CFB>::CIPHERS = {
    hex2bin("f6d1437d874cebb0fe8389fe84a00f3e"), hex2bin("b14eaebc619ecd0190b8924cadc65dfa"),
    hex2bin("fe8c7ecb9fafd4cb10382677f920d204"), hex2bin("492952cbe6f15407f4b4cf75f7ce689a"),
    hex2bin("1c8789a244fc5d77d3095247b8e6723e"), hex2bin("076802743c1e2b16de464c5eb4cdd8fb"),
    hex2bin("ded744d16f76812d6eac82124c0c68d6"), hex2bin("ab8433aa58b4109ae6b2bf868df3cfe7"),
    hex2bin("9e02444d43f4e2bb360ccdd2b673d436"), hex2bin("0d09b9821107e1f00a9a385d689efe39"),
    hex2bin("232647ce0b8931e983eaecdd8e57db6e"), hex2bin("b490bfcc3733fbd4492b8a3eccbedfe9"),
    hex2bin("896e7076237bcde1046768b064306bd6"), hex2bin("685359cce1319a93c9269938c2a91197"),
    hex2bin("aa09cb57d63e82a83d7931c3300a0e66"), hex2bin("3647bd48b4a2a523d70736916a1b0844"),
    hex2bin("da6779bf762fe49f66a3df8df5d37015"), hex2bin("9c66f290066778cf063e234ae3ec364d"),
    hex2bin("e6423b96eabd392399d32392867f54a2"), hex2bin("fde7f3654ba3499a7fa8377ea54653d6"),
    hex2bin("0f9d9ac430f9b80f43c22cf0e26be80d"), hex2bin("949b5e80e66c749f3d2b9cb2b7fcc044"),
    hex2bin("6c5df2d6f7b662256d8a8c521d3e00dc"), hex2bin("f10802dee5109f")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_CFB>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CAMELLIA_128_CFB>::CIPHERS = {
    hex2bin("3e58a3a3dab4834e06179c3ea4d6e047"), hex2bin("1a0b50ce7841e8321e4abaf037b9e719"),
    hex2bin("170a0994ea8a254ccba3aa1d57a5b3fd"), hex2bin("5cdaf877520b10664347e4724159e508"),
    hex2bin("633b5712a2ed2d0615091808e1d0a88a"), hex2bin("2ec9efbc1d3a19c230497093a8e24e8f"),
    hex2bin("d8e7b20547450867e3f27592671a9967"), hex2bin("609fa87e34dde5d9f0cc82ad72d56167"),
    hex2bin("c1e0474264a322ec89762283073278fa"), hex2bin("a45156dd03a2dd5b0c3f4fee3396b0af"),
    hex2bin("86d11ef0ad33a46ef48cb82cb3935723"), hex2bin("a6a0548db6f46f98da5d6fe4df81b7e6"),
    hex2bin("015686e6a1bf36b588a038a6cbf4fdbd"), hex2bin("46ec956b992f44f137f6d995f884aee9"),
    hex2bin("e717f8d6f960b057c5d625ae5dad15dc"), hex2bin("c3debb098da3c1fdb08101c70a6aba63"),
    hex2bin("3cbb74499017b87238a3635f5e68b0d1"), hex2bin("e97bd3cb809cecfc2d792b577d671a43"),
    hex2bin("a7035137a3622058b13e6e7558a4e13f"), hex2bin("0a474547cd24445fe7665287ff4a4606"),
    hex2bin("c1965d73d0397ab6d9b0e9e4c48156e8"), hex2bin("03b0309168b9fcd687f75df1018748fa"),
    hex2bin("ac983af22127106b63320ca3aad5fe1a"), hex2bin("5ac25d51eccb3d")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_128_CFB>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_128_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CAMELLIA_192_CFB>::CIPHERS = {
    hex2bin("e29d7c55dd5280510297bbfe11306100"), hex2bin("3a682cf5803c757a55aea6e4ad052513"),
    hex2bin("09b5a3a1ac50ca4b947312016a570aab"), hex2bin("523784b76681d14919a02a8dc4b034b6"),
    hex2bin("a5768a73017c3abd8d2702972aa01c80"), hex2bin("356677508e79a51f53009fe9cc111aa8"),
    hex2bin("122ef0e7f410c4398d456b6b6354e950"), hex2bin("8e5d20b1e4994ebf3b2a5ae5cd3eb036"),
    hex2bin("e74e3e45c468afcc34d1a0e0e52c6a6d"), hex2bin("1a73cb7569bbc3003e27468c4bcc4d25"),
    hex2bin("2e1603fd4e51d932120227f62d294ed5"), hex2bin("baaf1752e3aa3fa005626b13610185a6"),
    hex2bin("9a8f7ba2ed20f1be14c2edee1af3b06d"), hex2bin("b0ff40dbd44087638228baeb800c976d"),
    hex2bin("676e7200ec624fb01321112d6df3b365"), hex2bin("7c3023696a57ef4848967c5b5b3a311a"),
    hex2bin("e455a31eb0b7155f749d2269a2001b26"), hex2bin("3a159ccb9ef977ac8584797283838d99"),
    hex2bin("3803a592e9b012de4ea7c02344360df2"), hex2bin("279117bd9a779aee812b9dfb69034ed3"),
    hex2bin("8f24a3fc31ac01a49eca1ad7cbc5a38f"), hex2bin("746ad1a77dbbd99d85905cbb929b1780"),
    hex2bin("d34de30a185f5ae6f23f0d3396e04de6"), hex2bin("efda299c61aaff")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_192_CFB>::KEY = KeyIv<24>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_192_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CAMELLIA_256_CFB>::CIPHERS = {
    hex2bin("e5cec07951df808431f33c8dfb4d5bcd"), hex2bin("7f77e7a5310d323678c5c16776d744d3"),
    hex2bin("e3c6107ebb2de5fce36314c7023637b8"), hex2bin("9aba524a1113515a5a5a06a5f0e71931"),
    hex2bin("e963039b23e60bfeb1bff68f2facfd15"), hex2bin("a917601ada1b4735ae13b15db5872f13"),
    hex2bin("f2c045363c0fd7059b99a555fcfb64da"), hex2bin("6f527bbfbb503433dd5854664037e2e8"),
    hex2bin("7eb77c4455953c0f690558e0929e1587"), hex2bin("47c6998ee0dc668da25abea3ded3de6a"),
    hex2bin("0292e6bac37dc33eb8d5af2cf794b2ee"), hex2bin("55fcaf59ab64978aed924e1bb5df43c7"),
    hex2bin("488dee0c3421742837bff039cff4c6dc"), hex2bin("2ca49734e9d1d4664434946144324392"),
    hex2bin("d2b667b53f1aec3bc38f49d0c20d0fc3"), hex2bin("6031bf0279ae83d9369ebad2c29c5ff7"),
    hex2bin("54bd5dadad0c5a50c40978c4734fc7a2"), hex2bin("b3dd98081696705ec974476fa2896810"),
    hex2bin("4a824c9b1e00571610cc2166fc19e82f"), hex2bin("e56851ab387fcf5a764b67018955563c"),
    hex2bin("35fd037cc08d4b83089e93cf1ccecc5a"), hex2bin("c13796e32a8488e504384a579d3df7c7"),
    hex2bin("de7a34fef8acf457889aa18083e1f655"), hex2bin("d36d2ae18a80bd")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_256_CFB>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CAMELLIA_256_CFB>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CHACHA20>::CIPHERS = {
    hex2bin("1684ccd15bec3a45301836ab3165ab04"), hex2bin("867c1788399e63e5e3fe561bcc07bf9b"),
    hex2bin("0ceb6a476a7bc39733f4a80b848fc3a0"), hex2bin("02fc6012cb1f94a1a9e0ed89130a937d"),
    hex2bin("0e40910b5843e3579667fe8389e762f8"), hex2bin("6587e55c2e97523a3a0686f27c181ad5"),
    hex2bin("00167c51cf5ff86a431129668baaf4e0"), hex2bin("6f59bfff61ce32f329364161a8032892"),
    hex2bin("ad8b40048e7f54886d62a50de265a82c"), hex2bin("c198f1509d47ea98267f13dc6a2e9147"),
    hex2bin("aee7942310ae22e15d687c53c9ee3a87"), hex2bin("996978f92a0b8590da7aab89a02789bb"),
    hex2bin("9424097138006ed02901b5f7d12e8754"), hex2bin("6af896f0e93525232e2f868f43b88421"),
    hex2bin("31439f3fbdf8da872aa788216adf84f2"), hex2bin("b3da8de62fd0a8a273209570ea8d69cf"),
    hex2bin("1da98316ae85d168cc705e87f87bcbde"), hex2bin("3af5d8901f8d75faf892b295fd4db159"),
    hex2bin("56b7e8fca105f06f53396d381c2043c9"), hex2bin("4cc99e1c704e74818c66505f9034358f"),
    hex2bin("b6cbe56b8b04c1987dbaede5cea3daa6"), hex2bin("7f160171471f8e0297cf5c40195355dd"),
    hex2bin("a6913f4bb1160ecbb288701ee4ed1a97"), hex2bin("d98dee59bec4cf")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20>::IV = KeyIv<8>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::SALSA20>::CIPHERS = {
    hex2bin("250498406d6a3ea80c0ac5db1527f387"), hex2bin("66579d12a504a9af945ba0644da6914d"),
    hex2bin("3353b821d49ee0bc54883700def7959a"), hex2bin("c516c1df50e48241ef3e66760d382c03"),
    hex2bin("acdb14089379e018f335674f32342c3d"), hex2bin("c7c2d6c37d7cfe5a3719fe735a72e0a6"),
    hex2bin("4097b8df36142aa92f15506369987288"), hex2bin("9b5cc338128d8b5268add08d9e15d4e7"),
    hex2bin("6663772866b1850c4de8805a5f2a4909"), hex2bin("913747d98cbd5a07b0841ea254d09281"),
    hex2bin("3c6cd4452ac43dba02cf2a196e186a2d"), hex2bin("194c1619b2411103d3c9d0d453d8b979"),
    hex2bin("4aa3323ac2f13ba61ae0f5f32b48feb8"), hex2bin("cbbb3c0e7c3b12e6552f4e26adca55a9"),
    hex2bin("52d67a62bd913ac157810e36597d93f4"), hex2bin("b2c269767790ffae8044091e813f806e"),
    hex2bin("829c578ed083c5879ea400c7affacfb7"), hex2bin("4683c815d46081030a0922ae43bd903f"),
    hex2bin("2be4e103e84c513364e4f01fdac4703c"), hex2bin("4eeb79efb341c0af4bd2ade254018760"),
    hex2bin("4b9e3270e1a04f770cf637cc5ac72e52"), hex2bin("fe828faea2b2dd65d6ac3f6912e6ede1"),
    hex2bin("2d90fd9e411460e67022ba9a1b36f24a"), hex2bin("1fb995f025ebd3")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::SALSA20>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::SALSA20>::IV = KeyIv<8>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CHACHA20_IETF>::CIPHERS = {
    hex2bin("2e94bc50ba0c3479d767cc6f848a5175"), hex2bin("3005de21029e6aa5c806e4bf9402d121"),
    hex2bin("98e5e32f8acb9425e35185c12c02469f"), hex2bin("3b80fba4f83a3ce18c352910eed390d6"),
    hex2bin("65ad18cdbe2f694d5a79d390d50ec820"), hex2bin("a31c3d9695ffb651b75daee0e3746447"),
    hex2bin("e8047f5121ed0644fe5c8cbd6fddf5df"), hex2bin("55dca95bfcab4079247a754b142739eb"),
    hex2bin("84c1f4c4315dbe32f83a49e7b8db6679"), hex2bin("b6806539cdeba3a69f94d6f736e3265a"),
    hex2bin("e2687c1f3485a76dbcc8204670f6a96e"), hex2bin("9fd2bc2959ed0de80498047bbc70aecf"),
    hex2bin("ebc9c2d9036885abc439fbcab2c9447a"), hex2bin("c9ecb10fd556e89265c64ee1c5f97f87"),
    hex2bin("dba8099bf0391654b34f4570540a41fa"), hex2bin("fab8347122bfee7c3dcf14389b3ffc4f"),
    hex2bin("cc70d70f22402be039ab7feac0d3a1ca"), hex2bin("e06014cf6cf10fbe1f52155666e0e7b1"),
    hex2bin("f7f8c098f9f0916cc74799a9ba572e41"), hex2bin("fe524348377a5a1e6a5fa82155e3dac1"),
    hex2bin("e3ca7127c78492278b78313177d05219"), hex2bin("e413a58888d33d51be4586cf60dfbdaa"),
    hex2bin("dfbc8cb0071697c0c2bae0dfe7b508b9"), hex2bin("780ae5b4611b77")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20_IETF>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20_IETF>::IV = KeyIv<12>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_128_GCM>::CIPHERS = {
    hex2bin("c9c277819cb0011569cd5b0e85c73efa84925a5438106a7ff430e7a20f85fa36"),
    hex2bin("c0973cc85991bb9c1abd96b537434960bdbda3a6288e679170d7fb45143465f1"),
    hex2bin("865cc6a08a2566507f107d96004739297158cbbe3f1e768e069a2dd04933cd85"),
    hex2bin("56f9e155454ca04f6ece2f11b031a78a4301ffec7ef0ebad91a197816e7faf14"),
    hex2bin("2a4fca209b81a235a03d6e32a9ece3af1bf6c068636f2dbcbf66ba5f586d197f"),
    hex2bin("a89d2af46c0ed85be2ebacda0d8cbe71ad2b6176435af6829249f6f43aac3c02"),
    hex2bin("36913fe19008ef7e80ebc9e8c0ac183154904bcf08135a1596054756bff0f2af"),
    hex2bin("821a3544f5153a1e9a8e6e08f3548b6650248a53f92875571547663727559b7c"),
    hex2bin("b47fa5c3c288f7de5bcabd30b122649992d1a9d5f5dd67cd00844d05aee025e7"),
    hex2bin("59e0f36c9d84109f0b955fd4be55ea6b6a470179e2481d32c527a67f3d840608"),
    hex2bin("993a1bbad0740612584e5cdd8ba0b8bb9f265b309c25b917dec558a393cc6ab5"),
    hex2bin("66369f34ae7da820fe640f0c6129f7b84f520b0d7d01a3a73fa77933efddf0d0"),
    hex2bin("235aab6605e17f61d5a7d4f0637f8b4a8925b229dd71ceda94e0bb2a61298dd0"),
    hex2bin("cc0ecb11a4c690cb9c31ad76f2c453c1e69a59e1e2eeb1daa6573aeb52df0357"),
    hex2bin("da2e54cd6e8df1407097cf10b6946220cdcfca69528eabe168f72e0b931951b8"),
    hex2bin("62a4190fb60429d6727d1aaa8c84dfab4fb4a1082708747b6dae1d64befdd770"),
    hex2bin("71702ee9a5ceb51347401068fb5b15cc138e158df22271c2925e3b117be33fb4"),
    hex2bin("a915998bda3ee8883153a2aa517372161116804a82ca36fd122957c8dd0388de"),
    hex2bin("b46a4ee570e9b7be3b288f3962eab65a6b4b06a75f645183b6d90e621d5fb2a0"),
    hex2bin("57ab84e8e08acd2624cbbfbef7a364da335bf23a51ed9473d75f609e7fa866ec"),
    hex2bin("5d8f28a353068dfc0b0fa3c7883b45131a2fee8ad8655f05d9f4defb34d79052"),
    hex2bin("8f8e817d6b43599cf0f98fd2ca3e52886e7ce4e1637c639411ce486c9107bcf9"),
    hex2bin("725084727e9ed0709a7542ab208dad691fbdda66258acebbbf67c01ae18f70d4"),
    hex2bin("10dff127a36fa0f950de2d44f5add904b5907ced4dcde8")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_GCM>::KEY = KeyIv<16>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_128_GCM>::IV = KeyIv<16>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_192_GCM>::CIPHERS = {
    hex2bin("7c19e0891d50742db3ca83dbe89502cb59e5c9ee061729dc7644d18a1f649e24"),
    hex2bin("dc3719ca5d20e564de98acf67faa139b69500605194596c687b021d61342ef71"),
    hex2bin("b79e259c9eb8ba0879f81dc7d1877835ec67f1d54aa550ea4f972df61007364d"),
    hex2bin("fd9ab1c8288e3aa36936f6b51528630ccb0710ad799d72d0c108c851292f0d62"),
    hex2bin("56ca85b7eca596d60db7c63cee039c5b539e68412047839a0fb9df883962bf20"),
    hex2bin("792a7f92f4e0b808aa1ff23bc1f0774ca85a416b096464ccd7c6377fcc388ae4"),
    hex2bin("39aaccbf8b600d3cf0b4baff80250f31d794c6e145f5d46c7cae7f83dfd27338"),
    hex2bin("560c53898a30d50c60f9b7c45164e808950f360a5192d9bf96041c221840cdec"),
    hex2bin("ad29c920d59956266ad2ed525eac421a5f5e911b73aa22698db0bd06e713b34f"),
    hex2bin("98b6b850b0fe73acd2e1ebe2a00b380061d02822c8e974c3a0cacdcfc6af1867"),
    hex2bin("0f573249e6b8944cfc5ac060bef0ac30f07b1f1416f7bd8cd780cc8aacc16119"),
    hex2bin("592f1d1c6ae13edf1e6f8c6af7fa90ff2bedc0821345712b9d78757e56a7298a"),
    hex2bin("373d27d9e073486c59a882d0002bb2f3b1decc45be7716248129db2f16b07551"),
    hex2bin("70536706913adc244ab2c7e4c839edb9a2bec10312e9d84705ef8e7a991cd80d"),
    hex2bin("a444281a74515056617a11ec792dd501795e07f7998fd3b67f20ee760ffe3969"),
    hex2bin("cd8e2633ef937150e19b4ea46ce8d11182f76453409bec76298c3cdb6f9e7c7d"),
    hex2bin("6bea4ba0735a8f8e3dda81cedce20cc49cd83eac91793983f4488265c0f463a6"),
    hex2bin("6ef6e331e3d4c56e9d830f2570038af0123c0a69ac89146c5358cd1285711b0e"),
    hex2bin("ca6c6a35ff22929267e34eb78c0cda88a753be9e64191a0b17ed11474b7d74c7"),
    hex2bin("62316401d49d13da7fc9060c8d252e943b3cd9b248bd5bdefad49f7261cfce77"),
    hex2bin("5df81f57e4964422b516ded225e362794faf91f2a71804c14779d8ddf7f4aac3"),
    hex2bin("1e2bf96f16e56fa756ec34e91b4f1ae681c42df1a6d4db95973c543b951f7eb3"),
    hex2bin("3b2493a09cba58d77e8926ab4ff49d970e6f68d2899d34f208069f9c490171b6"),
    hex2bin("63db077e4cf27cb138ec76163b1d4f591165c23c8947be")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_GCM>::KEY = KeyIv<24>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_192_GCM>::IV = KeyIv<24>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::AES_256_GCM>::CIPHERS = {
    hex2bin("ccd6732abb6e0e07a5fe672603cbc0bc11d0b003f65d1d245865a150d463fcd9"),
    hex2bin("82dedef4ad32d746c6544962459f54ec6fc4d15008b66e9b81949e2a59ec75ca"),
    hex2bin("cda72230cd8f5b84a1774388c087a5fcbe98becfca47ad02c4737765720fa393"),
    hex2bin("0b79bee0a526586f792c91a83b7c321b7992a22e9e0bb9c44b9fd0b874e430ec"),
    hex2bin("6172649b3b51b5e2da75424cd7a50c6d3662f696c1e60dddd76cb50eb947615a"),
    hex2bin("2fab9823313a6f06542a5701b19328c01ee9fa9eed74a5ce0587bfb219bc2f82"),
    hex2bin("1c7bae8be9b5322ac7a30066d7f15439df0a8897e0573768d6a41631b763f983"),
    hex2bin("ab02a49c76af28b8ed2efb172e4ad5fa70864dbd820cdc35d2e0f7185ecf7f66"),
    hex2bin("f364504fc65075849a9c10de2b590234f7764ce178943c515d65992dfc70ed14"),
    hex2bin("71f141b4c88bdbf0cca705f212a0549e79f2c07564843402611b2a3fd81cb297"),
    hex2bin("bf2206fd03b9baf74ffc53dff9c636bfdae595cd7e7e172c6d0b9afa5c9241b6"),
    hex2bin("cbd41f1290776752b43904a1e224fd7d20cc723b8c603fd9b378d4fcb6dcba9f"),
    hex2bin("98bff28e2b8146abe29ef588e5ee68a9f45f6c67b78d96411d0b749d434bd98b"),
    hex2bin("157221252bb6db76d9c4d74caad0d57c26a08ef437e9b85096a80d4c97413221"),
    hex2bin("a89ec2badf03815254a1a152f15af35d667be2056310bcdc1573d2c28b971e6d"),
    hex2bin("e89d76b7ce069a11e5236d34b5379c06f91fbb6fb3dced3ab29d36867cb3fcab"),
    hex2bin("b29b19ade9006d5ce110eeaf74c89ea1eea747f8da524e9d5bcf9ef22584f595"),
    hex2bin("db93d370b41f18f4125d91f2a89111e008cdd721ff69dff89149a28ae5ce9f61"),
    hex2bin("8565dd021c475d44914e19a2f3c80ce6559d20f569301a0d28fd245d43e62b0a"),
    hex2bin("064251ae39f90a94c43f0a191769e744d0b8df8de630f60905cdcdcd33978768"),
    hex2bin("7ebd96b1482185348ecd9b25ea03e127753a3881b8b3c958ff7fb11b8a749c39"),
    hex2bin("bf7d5b796a26bb99aaf10fbfbc932b9eed93ddeafa4c17d707f54cc9eefe1b02"),
    hex2bin("019b29de35335d177dd58900ffade7c3c7af306a651cb08412a6096ce1acf00c"),
    hex2bin("c736da2fbbe7f7e89cea970a0cf947a8cb101cd172b8d1")};
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_GCM>::KEY = KeyIv<32>::KEY;
template <> vector<uint8_t> const& Ciphers<CryptoMethod::AES_256_GCM>::IV = KeyIv<32>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::CHACHA20_IETF_POLY1305>::CIPHERS = {
    hex2bin("b7666c4515ad64195661bd36abad76db148d2a61921d438cca6f16f50be384de"),
    hex2bin("890ea6494c7273868b0d8464ecb24a1ae32fcc0c7ea5f1e5b43fcac5b7879f95"),
    hex2bin("7365716a5181fed9344175395696f28b274f3366794f54f6e195ccf65cb170de"),
    hex2bin("947092e8e2e9709f433f51b30457189d2041f197fb56d27bf97f3ae5db063bc1"),
    hex2bin("2c24f64a324e8224f078d2e20268536b461c9566f4be6eadc3168f6d58246ac1"),
    hex2bin("acc791985627c625f0e866fa7ba41c4cd8909ec20399da7a9fcbc86bb0dfef40"),
    hex2bin("6939520dee1e411e17fe51e50a2c18e66de0752982d916041f5104776d6da5c6"),
    hex2bin("e91cecf7746265c99785b9fc03ba4a0efcffffeaf3678c37f6d391414db61180"),
    hex2bin("fdd2d8fbe966941d44bf6417a5882b4203285a2d51360119d13fd48b0acc3a7f"),
    hex2bin("97a6493da0b3395da15e6965e2d0e29de093659bf463ea6e7265379518b8a556"),
    hex2bin("d038415640c66d3ec37c6f65173ad948441b449b277677149c42201a6411338f"),
    hex2bin("3c6d9bdc5c7db74e4fc5060dd17a527d0073da7e55aa6bb36c0ee3f7dc409408"),
    hex2bin("db3e2b8d0a22ef38e90459eb1ee018ada1e7fbeab08c04ad8640183e8362657a"),
    hex2bin("68197f6606968ecbdbb94997336322539ffec026c127072a1c27ac23c95fa8f1"),
    hex2bin("2db5777ec2e0bc08d481bf5d6aeed3c1769b19fb0ebb2d6b2aba1435df45fe7d"),
    hex2bin("239a9c7147ae0c263509bab4407fa5f5f9a642c3704fccb3679f1c46f13bd4c8"),
    hex2bin("2ae40a083121424af4aa291f6e8b4ae984ddc49a9d76b0c67fcbf1256e56d3ff"),
    hex2bin("324721b85e6d44bbe72740252429d38da7a6a7149b787bb026982c6e13f29285"),
    hex2bin("16ea150f501bcee7e0615152ea047641c242de37a3e73bafaf2c060b3c99dff9"),
    hex2bin("6527c371458247f633d276980ef79b8eb1912c367bab8eebd8c0594ef3c6f21f"),
    hex2bin("f9fce82375fc654d95ae4cc9e3dd6b308156a353eb48c5b64c20afbef18d7ea2"),
    hex2bin("4257a726b5725e897f1b4555afbdb2de84b579acf95e76ba2ab0b75304cd0222"),
    hex2bin("972dd8a1e8c13f892b9a18d29d3516bd1d05f3c884be808dfad7e606c1333945"),
    hex2bin("6d9aab8c457b53d09f5d6fa8b1bf36d392b64b3f08e3d1")};
template <>
vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20_IETF_POLY1305>::KEY = KeyIv<32>::KEY;
template <>
vector<uint8_t> const& Ciphers<CryptoMethod::CHACHA20_IETF_POLY1305>::IV = KeyIv<32>::IV;

template <>
vector<vector<uint8_t>> const Ciphers<CryptoMethod::XCHACHA20_IETF_POLY1305>::CIPHERS = {
    hex2bin("0aeb53395cfac7858af40e9fe1d7776f48232d3ee0b09693d19540bc7a84fc87"),
    hex2bin("c8469fc8b1d7a9aef1f84d1b031353591bf199151fac9349e913f1cf3bed37a9"),
    hex2bin("c44f84bc1a7aeb3e281d5d07d04b195babf445ab956b04667d442503c37489cc"),
    hex2bin("0f92fb56e9977305665201091d74b5e720848028b1e3d55a5b30905fe2daf2b0"),
    hex2bin("168a8ca2e81a02287d1384c9f1483353d0e7294bcca07b7b35c650ab8f03faef"),
    hex2bin("33f399f1ccb5785c5c9efc331c5a335c7d553cc7b4d8d80dd1f48a85ac5157aa"),
    hex2bin("219f652594de9ddec69e4e57af737bf81cd966509bb424b433c2c14fe866977b"),
    hex2bin("7989fbf62941fd87d4f329675ff7848840fec61d3d2af6ab7280baae60dd8423"),
    hex2bin("ac08f09104a6c582b2ee9f0a72911277983df1a89929bb66686ca424ae1f932c"),
    hex2bin("0bde0787f4155779eb7e7951cec5cd08299c736a78fbf31477ac138fe68ace86"),
    hex2bin("1df3a23d80c9105451d20fed2672838de005459ee673b1d6f7721b55f9773e66"),
    hex2bin("6ed60017c6b64f70a564d635afbe2365f1fed003b5662abdb98eaa07fe0a85cb"),
    hex2bin("75ccee276507ebb5f25252de85715a75994869311c9368775794af1e12fdc38a"),
    hex2bin("4b681e525bd87c9f82ed682a58f80dd494670b82527cd90bee28cc0c7bbebd47"),
    hex2bin("40b6fc99fa55bbeeaea7255c828a5b64fc3efe5a5ed9f61a814bd52ec35dd78a"),
    hex2bin("b4115fef0af5da2e87bb5cbe7b9e99ea8ccc693c5a1cdad67b951d2669941f7e"),
    hex2bin("3583ead7b8f75381b2cd2cfe52be081d42bf14fb769796944c4ec4066586213b"),
    hex2bin("fb2bc8c89d6b97d44bfdbd16ce7fa8ab8ba6990eca4f86c878a0c68aa245fa49"),
    hex2bin("c4cb515759dd881eb813f2cca4e18f90f102ccd162b4b3c978f898a02bfff97a"),
    hex2bin("2eac3421523880690830521b6fe4eff44c9ced292b935e362ec41593b2384890"),
    hex2bin("956e9edd2456a48fa4cdd833fa65dcf5d54958e6742efb90ddcb77da5a600dc1"),
    hex2bin("0ca4b820194ac5aa4f2cf4459c0843fe987a7eef71aa21a6032dbe97ed389e4b"),
    hex2bin("fed7d829c2f957fb11716b803fb17003b6c42aa387aaf2397d6584ee58cdc0e2"),
    hex2bin("47f8260829e5968749f318113e3cce60ad910dce1dd695")};
template <>
vector<uint8_t> const& Ciphers<CryptoMethod::XCHACHA20_IETF_POLY1305>::KEY = KeyIv<32>::KEY;
template <>
vector<uint8_t> const& Ciphers<CryptoMethod::XCHACHA20_IETF_POLY1305>::IV = KeyIv<32>::IV;

using Cases = mpl::list<
#if MBEDTLS_VERSION_MAJOR < 3
    Ciphers<CryptoMethod::RC4_MD5>, Ciphers<CryptoMethod::BF_CFB>,
#endif  // MBEDTLS_VERSION_MAJOR < 3
    Ciphers<CryptoMethod::AES_128_CTR>, Ciphers<CryptoMethod::AES_192_CTR>,
    Ciphers<CryptoMethod::AES_256_CTR>, Ciphers<CryptoMethod::AES_128_CFB>,
    Ciphers<CryptoMethod::AES_192_CFB>, Ciphers<CryptoMethod::AES_256_CFB>,
    Ciphers<CryptoMethod::CAMELLIA_128_CFB>, Ciphers<CryptoMethod::CAMELLIA_192_CFB>,
    Ciphers<CryptoMethod::CAMELLIA_256_CFB>, Ciphers<CryptoMethod::CHACHA20>,
    Ciphers<CryptoMethod::SALSA20>, Ciphers<CryptoMethod::CHACHA20_IETF>,
    Ciphers<CryptoMethod::AES_128_GCM>, Ciphers<CryptoMethod::AES_192_GCM>,
    Ciphers<CryptoMethod::AES_256_GCM>, Ciphers<CryptoMethod::CHACHA20_IETF_POLY1305>,
    Ciphers<CryptoMethod::XCHACHA20_IETF_POLY1305>>;

using StreamCases = mpl::list<
#if MBEDTLS_VERSION_MAJOR < 3
    Ciphers<CryptoMethod::RC4_MD5>, Ciphers<CryptoMethod::BF_CFB>,
#endif  // MBEDTLS_VERSION_MAJOR < 3
    Ciphers<CryptoMethod::AES_128_CTR>, Ciphers<CryptoMethod::AES_192_CTR>,
    Ciphers<CryptoMethod::AES_256_CTR>, Ciphers<CryptoMethod::AES_128_CFB>,
    Ciphers<CryptoMethod::AES_192_CFB>, Ciphers<CryptoMethod::AES_256_CFB>,
    Ciphers<CryptoMethod::CAMELLIA_128_CFB>, Ciphers<CryptoMethod::CAMELLIA_192_CFB>,
    Ciphers<CryptoMethod::CAMELLIA_256_CFB>, Ciphers<CryptoMethod::CHACHA20>,
    Ciphers<CryptoMethod::SALSA20>, Ciphers<CryptoMethod::CHACHA20_IETF>>;

BOOST_AUTO_TEST_SUITE(Cryptogram)

BOOST_AUTO_TEST_CASE_TEMPLATE(Encryption_short, Case, Cases)
{
  auto encryptor = Encryptor<Case::METHOD>{Case::KEY, Case::IV};
  BOOST_CHECK_EQUAL(plains.size(), Case::CIPHERS.size());
  for_each(cbegin(plains), cend(plains),
           [&encryptor, cipher = cbegin(Case::CIPHERS)](auto&& plain) mutable {
             auto fact = vector<uint8_t>(cipher->size(), 0);
             BOOST_CHECK_EQUAL(cipher->size(), encryptor.encrypt(plain, fact));
             BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(*cipher), cend(*cipher), cbegin(fact),
                                           cend(fact));
             ++cipher;
           });
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Decryption_short, Case, Cases)
{
  auto decryptor = Decryptor<Case::METHOD>{Case::KEY};
  decryptor.setIv(Case::IV);
  BOOST_CHECK_EQUAL(plains.size(), Case::CIPHERS.size());
  for_each(cbegin(plains), cend(plains),
           [&decryptor, cipher = cbegin(Case::CIPHERS)](auto&& plain) mutable {
             auto fact = vector<uint8_t>(plain.size(), 0);
             BOOST_CHECK_EQUAL(plain.size(), decryptor.decrypt(*cipher, fact));
             BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(plain), cend(plain), cbegin(fact), cend(fact));
             ++cipher;
           });
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Encryption_long, Case, StreamCases)
{
  auto encryptor = Encryptor<Case::METHOD>{Case::KEY, Case::IV};
  auto plain = accumulate(cbegin(plains), cend(plains), vector<uint8_t>{}, [](auto&& s, auto&& i) {
    s.insert(end(s), cbegin(i), cend(i));
    return move(s);
  });
  auto expect = accumulate(cbegin(Case::CIPHERS), cend(Case::CIPHERS), vector<uint8_t>{},
                           [](auto&& s, auto&& i) {
                             s.insert(end(s), cbegin(i), cend(i));
                             return move(s);
                           });

  auto fact = vector<uint8_t>(expect.size(), 0);
  BOOST_CHECK_EQUAL(expect.size(), encryptor.encrypt(plain, fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Decryption_long, Case, StreamCases)
{
  auto decryptor = Decryptor<Case::METHOD>{Case::KEY};
  decryptor.setIv(Case::IV);
  auto cipher = accumulate(cbegin(Case::CIPHERS), cend(Case::CIPHERS), vector<uint8_t>{},
                           [](auto&& s, auto&& i) {
                             s.insert(end(s), cbegin(i), cend(i));
                             return move(s);
                           });
  auto expect = accumulate(cbegin(plains), cend(plains), vector<uint8_t>{}, [](auto&& s, auto&& i) {
    s.insert(end(s), cbegin(i), cend(i));
    return move(s);
  });

  auto fact = vector<uint8_t>(expect.size(), 0);
  BOOST_CHECK_EQUAL(expect.size(), decryptor.decrypt(cipher, fact));
  BOOST_CHECK_EQUAL_COLLECTIONS(cbegin(expect), cend(expect), cbegin(fact), cend(fact));
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace pichi::unit_test
