#pragma once

#include "api/meta_protocol_proxy/filters/local_ratelimit/v1alpha/local_rls.pb.h"
#include "api/meta_protocol_proxy/filters/local_ratelimit/v1alpha/local_rls.pb.validate.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

class LocalRateLimitFilterConfig
    : public FactoryBase<aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit> {
      
public:
  LocalRateLimitFilterConfig() : FactoryBase("aeraki.meta_protocol.filters.local_ratelimit") {}

private:
  FilterFactoryCb createFilterFactoryFromProtoTyped(
      const aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit& proto_config,
      const std::string&, Server::Configuration::FactoryContext& context) override;  
};

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
