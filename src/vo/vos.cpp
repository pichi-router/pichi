#include <numeric>
#include <pichi/common/endpoint.hpp>
#include <pichi/common/literals.hpp>
#include <pichi/vo/keys.hpp>
#include <pichi/vo/messages.hpp>
#include <pichi/vo/parse.hpp>
#include <pichi/vo/to_json.hpp>
#include <pichi/vo/vos.hpp>

using namespace std;
namespace json = rapidjson;
using Allocator = json::Document::AllocatorType;

namespace pichi::vo {

json::Value toJson(ErrorVO const& evo, Allocator& alloc)
{
  using StringRef = json::Value::StringRefType;
  using SizeType = json::SizeType;

  auto error = json::Value{};
  error.SetObject();
  error.AddMember(error::MESSAGE,
                  StringRef{evo.message_.data(), static_cast<SizeType>(evo.message_.size())},
                  alloc);
  return error;
}

}  // namespace pichi::vo
