#pragma once

#include "api/meta_protocol_proxy/filters/global_ratelimit/v1alpha/global_ratelimit.pb.h"
#include "api/meta_protocol_proxy/filters/global_ratelimit/v1alpha/global_ratelimit.pb.validate.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace RateLimit {

class RateLimitFilterConfig
    : public FactoryBase<aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit> {
      
public:
  RateLimitFilterConfig() : FactoryBase("aeraki.meta_protocol.filters.ratelimit") {}

private:
  FilterFactoryCb createFilterFactoryFromProtoTyped(
      const aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit& proto_config,
      const std::string& stat_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace RateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
