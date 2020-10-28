#ifndef PICHI_VO_MESSAGES_HPP
#define PICHI_VO_MESSAGES_HPP

#include <string_view>

namespace pichi::vo::msg {

static std::string_view const OBJ_TYPE_ERROR = "JSON object required";
static std::string_view const ARY_TYPE_ERROR = "JSON array required";
static std::string_view const INT_TYPE_ERROR = "Integer required";
static std::string_view const STR_TYPE_ERROR = "String required";
static std::string_view const BOOL_TYPE_ERROR = "Boolean required";
static std::string_view const PAIR_TYPE_ERROR = "Pair type required";
static std::string_view const ARY_SIZE_ERROR = "Array size error";
static std::string_view const AT_INVALID = "Invalid adapter type string";
static std::string_view const CM_INVALID = "Invalid crypto method string";
static std::string_view const UINT16_INVALID = "Exceed the range of uint16_t [0, 65536)";
static std::string_view const DM_INVALID = "Invalid delay mode type string";
static std::string_view const DL_INVALID = "Delay time must be in range [0, 300]";
static std::string_view const BA_INVALID = "Invalid balance string";
static std::string_view const SEC_INVALID = "Invalid security string";
static std::string_view const STR_EMPTY = "Empty string";
static std::string_view const MISSING_TYPE_FIELD = "Missing type field";
static std::string_view const MISSING_HOST_FIELD = "Missing host field";
static std::string_view const MISSING_BIND_FIELD = "Missing bind field";
static std::string_view const MISSING_PORT_FIELD = "Missing port field";
static std::string_view const MISSING_METHOD_FIELD = "Missing method field";
static std::string_view const MISSING_UN_FIELD = "Missing username field";
static std::string_view const MISSING_PW_FIELD = "Missing password field";
static std::string_view const MISSING_MODE_FIELD = "Missing mode field";
static std::string_view const MISSING_CERT_FILE_FIELD = "Missing cert_file field";
static std::string_view const MISSING_KEY_FILE_FIELD = "Missing key_file field";
static std::string_view const MISSING_UUID_FIELD = "Missing uuid field";
static std::string_view const MISSING_REMOTE_FIELD = "Missing remote field";
static std::string_view const MISSING_PATH_FIELD = "Missing remote field";
static std::string_view const MISSING_DESTINATIONS_FIELD = "Missiong destinations field";
static std::string_view const MISSING_BALANCE_FIELD = "Missiong balance field";
static std::string_view const MISSING_OPTION_FIELD = "Missing option field";
static std::string_view const MISSING_TLS_FIELD = "Missing tls field";
static std::string_view const MISSING_CRED_FIELD = "Missing credential field";
static std::string_view const MISSING_SERVER_FIELD = "Missing server field";

static std::string_view const TOO_LONG_NAME_PASSWORD = "Name or password is too long";
static std::string_view const DUPLICATED_ITEMS = "Duplicated items";

}  // namespace pichi::vo::msg

#endif  // PICHI_VO_MESSAGES_HPP
