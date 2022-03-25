#pragma once

#include "absl/types/optional.h"

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
  virtual ~HashPolicy() = default;

  /**
   * @param metadata metadata used for generate hash
   * @return absl::optional<uint64_t> an optional hash value to route on. A hash value might not be
   * returned if for example the specified key does not exist in the metadata.
   */
  virtual absl::optional<uint64_t> generateHash(const Metadata& metadata) const PURE;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
