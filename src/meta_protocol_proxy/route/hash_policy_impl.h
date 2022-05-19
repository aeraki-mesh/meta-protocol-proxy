#pragma once

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/route/hash_policy.h"

#include "source/common/common/logger.h"

#include "google/protobuf/repeated_field.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

class HashPolicyImpl : public HashPolicy, public Logger::Loggable<Logger::Id::filter> {
public:
  HashPolicyImpl(const google::protobuf::RepeatedPtrField<std::string>& hash_policy)
      : hash_policy_(hash_policy){};

  absl::optional<uint64_t> generateHash(const Metadata& metadata) const override;

private:
  google::protobuf::RepeatedPtrField<std::string> hash_policy_;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
