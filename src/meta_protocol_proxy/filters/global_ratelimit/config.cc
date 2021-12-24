#include "src/meta_protocol_proxy/filters/global_ratelimit/config.h"

#include "envoy/registry/registry.h"
#include "src/meta_protocol_proxy/filters/global_ratelimit/ratelimit.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace RateLimit {

FilterFactoryCb RateLimitFilterConfig::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit& cfg, const std::string&,
    Server::Configuration::FactoryContext& context) {

  // cfg is changed
  return [cfg, &context](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(std::make_shared<RateLimit>(context.clusterManager(), cfg));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(RateLimitFilterConfig, NamedMetaProtocolFilterConfigFactory);

} // namespace RateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
