#include "src/meta_protocol_proxy/route/hash_policy_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

absl::optional<uint64_t> HashPolicyImpl::generateHash(const Metadata& metadata) const {
  absl::optional<uint64_t> hash;
  absl::InlinedVector<absl::string_view, 1> header_values;
  header_values.reserve(hash_policy_.size());
  for (auto it = hash_policy_.begin(); it < hash_policy_.end(); it++) {
    auto value = metadata.getString(*it);
    header_values.push_back(metadata.getString(*it));
    // Ensure generating same hash value for different order header values.
    // For example, generates the same hash value for {"foo","bar"} and {"bar","foo"}
    std::sort(header_values.begin(), header_values.end());
    hash = HashUtil::xxHash64(absl::MakeSpan(header_values));
  }
  return hash;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
