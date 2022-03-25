#pragma once

#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

/**
 * Request hash policy. I.e., if using a hashing load balancer, how a request should be hashed onto
 * an upstream host.
 */
class HashPolicy {
public:
  HashPolicy(const google::protobuf::RepeatedPtrField<std::string>& hash_policy)
      : hash_policy_(hash_policy){};
  virtual ~HashPolicy() = default;

  /**
   * @param metadata metadata used for generate hash
   * @return absl::optional<uint64_t> an optional hash value to route on. A hash value might not be
   * returned if for example the specified key does not exist in the metadata.
   */
  absl::optional<uint64_t> generateHash(const Metadata& metadata) const;

private:
  google::protobuf::RepeatedPtrField<std::string> hash_policy_;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
