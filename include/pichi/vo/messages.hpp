#ifndef PICHI_VO_MESSAGES_HPP
#define PICHI_VO_MESSAGES_HPP

#include <string_view>

namespace pichi::vo::msg {

inline std::string_view const OBJ_TYPE_ERROR = "JSON object required";
inline std::string_view const ARY_TYPE_ERROR = "JSON array required";
inline std::string_view const INT_TYPE_ERROR = "Integer required";
inline std::string_view const STR_TYPE_ERROR = "String required";
inline std::string_view const BOOL_TYPE_ERROR = "Boolean required";
inline std::string_view const PAIR_TYPE_ERROR = "Pair type required";
inline std::string_view const ARY_SIZE_ERROR = "Array size error";
inline std::string_view const AT_INVALID = "Invalid adapter type string";
inline std::string_view const CM_INVALID = "Invalid crypto method string";
inline std::string_view const UINT16_INVALID = "Exceed the range of uint16_t [0, 65536)";
inline std::string_view const DM_INVALID = "Invalid delay mode type string";
inline std::string_view const DL_INVALID = "Delay time must be in range [0, 300]";
inline std::string_view const BA_INVALID = "Invalid balance string";
inline std::string_view const SEC_INVALID = "Invalid security string";
inline std::string_view const STR_EMPTY = "Empty string";
inline std::string_view const MISSING_TYPE_FIELD = "Missing type field";
inline std::string_view const MISSING_HOST_FIELD = "Missing host field";
inline std::string_view const MISSING_BIND_FIELD = "Missing bind field";
inline std::string_view const MISSING_PORT_FIELD = "Missing port field";
inline std::string_view const MISSING_METHOD_FIELD = "Missing method field";
inline std::string_view const MISSING_UN_FIELD = "Missing username field";
inline std::string_view const MISSING_PW_FIELD = "Missing password field";
inline std::string_view const MISSING_MODE_FIELD = "Missing mode field";
inline std::string_view const MISSING_CERT_FILE_FIELD = "Missing cert_file field";
inline std::string_view const MISSING_KEY_FILE_FIELD = "Missing key_file field";
inline std::string_view const MISSING_UUID_FIELD = "Missing uuid field";
inline std::string_view const MISSING_REMOTE_FIELD = "Missing remote field";
inline std::string_view const MISSING_PATH_FIELD = "Missing remote field";
inline std::string_view const MISSING_DESTINATIONS_FIELD = "Missiong destinations field";
inline std::string_view const MISSING_BALANCE_FIELD = "Missiong balance field";
inline std::string_view const MISSING_OPTION_FIELD = "Missing option field";
inline std::string_view const MISSING_TLS_FIELD = "Missing tls field";
inline std::string_view const MISSING_CRED_FIELD = "Missing credential field";
inline std::string_view const MISSING_SERVER_FIELD = "Missing server field";

inline std::string_view const TOO_LONG_NAME_PASSWORD = "Name or password is too long";
inline std::string_view const DUPLICATED_ITEMS = "Duplicated items";

inline std::string_view const NOT_IMPLEMENTED = "Not implemented";

}  // namespace pichi::vo::msg

#endif  // PICHI_VO_MESSAGES_HPP
