#ifndef PICHI_VO_KEYS_HPP
#define PICHI_VO_KEYS_HPP

namespace pichi::vo {

namespace type {

inline decltype(auto) DIRECT = "direct";
inline decltype(auto) REJECT = "reject";
inline decltype(auto) SOCKS5 = "socks5";
inline decltype(auto) HTTP   = "http";
inline decltype(auto) SS     = "ss";
inline decltype(auto) TUNNEL = "tunnel";
inline decltype(auto) TROJAN = "trojan";
inline decltype(auto) TRANSP = "transparent";
inline decltype(auto) DUAL   = "dual";

}  // namespace type

namespace method {

inline decltype(auto) AES_128_GCM             = "aes-128-gcm";
inline decltype(auto) AES_192_GCM             = "aes-192-gcm";
inline decltype(auto) AES_256_GCM             = "aes-256-gcm";
inline decltype(auto) CHACHA20_IETF_POLY1305  = "chacha20-ietf-poly1305";
inline decltype(auto) XCHACHA20_IETF_POLY1305 = "xchacha20-ietf-poly1305";

}  // namespace method

namespace delay {

inline decltype(auto) RANDOM = "random";
inline decltype(auto) FIXED  = "fixed";

}  // namespace delay

namespace balance {

inline decltype(auto) RANDOM      = "random";
inline decltype(auto) ROUND_ROBIN = "round_robin";
inline decltype(auto) LEAST_CONN  = "least_conn";

}  // namespace balance

namespace security {

inline decltype(auto) AUTO                   = "auto";
inline decltype(auto) NONE                   = "none";
inline decltype(auto) CHACHA20_IETF_POLY1305 = "chacha20-ietf-poly1305";
inline decltype(auto) AES_128_GCM            = "aes-128-gcm";

}  // namespace security

namespace endpoint {

inline decltype(auto) HOST = "host";
inline decltype(auto) PORT = "port";

}  // namespace endpoint

namespace credential {

inline decltype(auto) USERNAME = "username";
inline decltype(auto) PASSWORD = "password";
inline decltype(auto) UUID     = "uuid";
inline decltype(auto) ALTER_ID = "alter_id";
inline decltype(auto) SECURITY = "security";

}  // namespace credential

namespace option {

inline decltype(auto) PASSWORD     = "password";
inline decltype(auto) METHOD       = "method";
inline decltype(auto) DESTINATIONS = "destinations";
inline decltype(auto) BALANCE      = "balance";
inline decltype(auto) MODE         = "mode";
inline decltype(auto) DELAY        = "delay";
inline decltype(auto) REMOTE       = "remote";

}  // namespace option

namespace tls {

inline decltype(auto) CERT_FILE   = "cert_file";
inline decltype(auto) KEY_FILE    = "key_file";
inline decltype(auto) INSECURE    = "insecure";
inline decltype(auto) CA_FILE     = "ca_file";
inline decltype(auto) SERVER_NAME = "server_name";
inline decltype(auto) SNI         = "sni";

}  // namespace tls

namespace websocket {

inline decltype(auto) PATH = "path";
inline decltype(auto) HOST = "host";

}  // namespace websocket

namespace ingress {

inline decltype(auto) TYPE        = "type";
inline decltype(auto) BIND        = "bind";
inline decltype(auto) OPTION      = "option";
inline decltype(auto) TLS         = "tls";
inline decltype(auto) CREDENTIALS = "credentials";
inline decltype(auto) WEBSOCKET   = "websocket";

}  // namespace ingress

namespace egress {

inline decltype(auto) TYPE       = "type";
inline decltype(auto) SERVER     = "server";
inline decltype(auto) CREDENTIAL = "credential";
inline decltype(auto) OPTION     = "option";
inline decltype(auto) TLS        = "tls";
inline decltype(auto) WEBSOCKET  = "websocket";

}  // namespace egress

namespace rule {

inline decltype(auto) RANGE       = "range";
inline decltype(auto) INGRESS     = "ingress_name";
inline decltype(auto) TYPE        = "ingress_type";
inline decltype(auto) PATTERN     = "pattern";
inline decltype(auto) DOMAIN_NAME = "domain";
inline decltype(auto) COUNTRY     = "country";

}  // namespace rule

namespace route {

inline decltype(auto) DEFAULT = "default";
inline decltype(auto) RULES   = "rules";

}  // namespace route

namespace error {

inline decltype(auto) MESSAGE = "message";

}  // namespace error

}  // namespace pichi::vo

#endif  // PICHI_VO_KEYS_HPP
