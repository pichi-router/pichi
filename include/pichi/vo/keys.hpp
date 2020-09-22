#ifndef PICHI_VO_KEYS_HPP
#define PICHI_VO_KEYS_HPP

#include <string_view>

namespace pichi::vo {

namespace type {

inline decltype(auto) DIRECT = "direct";
inline decltype(auto) REJECT = "reject";
inline decltype(auto) SOCKS5 = "socks5";
inline decltype(auto) HTTP = "http";
inline decltype(auto) SS = "ss";
inline decltype(auto) TUNNEL = "tunnel";
inline decltype(auto) TROJAN = "trojan";

}  // namespace type

namespace method {

inline decltype(auto) RC4_MD5 = "rc4-md5";
inline decltype(auto) BF_CFB = "bf-cfb";
inline decltype(auto) AES_128_CTR = "aes-128-ctr";
inline decltype(auto) AES_192_CTR = "aes-192-ctr";
inline decltype(auto) AES_256_CTR = "aes-256-ctr";
inline decltype(auto) AES_128_CFB = "aes-128-cfb";
inline decltype(auto) AES_192_CFB = "aes-192-cfb";
inline decltype(auto) AES_256_CFB = "aes-256-cfb";
inline decltype(auto) CAMELLIA_128_CFB = "camellia-128-cfb";
inline decltype(auto) CAMELLIA_192_CFB = "camellia-192-cfb";
inline decltype(auto) CAMELLIA_256_CFB = "camellia-256-cfb";
inline decltype(auto) CHACHA20 = "chacha20";
inline decltype(auto) SALSA20 = "salsa20";
inline decltype(auto) CHACHA20_IETF = "chacha20-ietf";
inline decltype(auto) AES_128_GCM = "aes-128-gcm";
inline decltype(auto) AES_192_GCM = "aes-192-gcm";
inline decltype(auto) AES_256_GCM = "aes-256-gcm";
inline decltype(auto) CHACHA20_IETF_POLY1305 = "chacha20-ietf-poly1305";
inline decltype(auto) XCHACHA20_IETF_POLY1305 = "xchacha20-ietf-poly1305";

}  // namespace method

namespace delay {

inline decltype(auto) RANDOM = "random";
inline decltype(auto) FIXED = "fixed";

}  // namespace delay

namespace balance {

inline decltype(auto) RANDOM = "random";
inline decltype(auto) ROUND_ROBIN = "round_robin";
inline decltype(auto) LEAST_CONN = "least_conn";

}  // namespace balance

namespace endpoint {

inline decltype(auto) HOST = "host";
inline decltype(auto) PORT = "port";

}  // namespace endpoint

namespace ingress {

inline decltype(auto) TYPE = "type";
inline decltype(auto) BIND = "bind";
inline decltype(auto) METHOD = "method";
inline decltype(auto) PASSWORD = "password";
inline decltype(auto) CREDENTIALS = "credentials";
inline decltype(auto) TLS = "tls";
inline decltype(auto) CERT_FILE = "cert_file";
inline decltype(auto) KEY_FILE = "key_file";
inline decltype(auto) DESTINATIONS = "destinations";
inline decltype(auto) BALANCE = "balance";
inline decltype(auto) REMOTE_HOST = "remote_host";
inline decltype(auto) REMOTE_PORT = "remote_port";
inline decltype(auto) PASSWORDS = "passwords";

}  // namespace ingress

namespace egress {

inline decltype(auto) TYPE = "type";
inline decltype(auto) HOST = "host";
inline decltype(auto) PORT = "port";
inline decltype(auto) METHOD = "method";
inline decltype(auto) PASSWORD = "password";
inline decltype(auto) MODE = "mode";
inline decltype(auto) DELAY = "delay";
inline decltype(auto) CREDENTIAL = "credential";
inline decltype(auto) TLS = "tls";
inline decltype(auto) INSECURE = "insecure";
inline decltype(auto) CA_FILE = "ca_file";

}  // namespace egress

namespace rule {

inline decltype(auto) RANGE = "range";
inline decltype(auto) INGRESS = "ingress_name";
inline decltype(auto) TYPE = "ingress_type";
inline decltype(auto) PATTERN = "pattern";
inline decltype(auto) DOMAIN_NAME = "domain";
inline decltype(auto) COUNTRY = "country";

}  // namespace rule

namespace route {

inline decltype(auto) DEFAULT = "default";
inline decltype(auto) RULES = "rules";

}  // namespace route

namespace error {

inline decltype(auto) MESSAGE = "message";

}  // namespace error

}  // namespace pichi::vo

#endif  // PICHI_VO_KEYS_HPP
