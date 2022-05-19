#include "src/meta_protocol_proxy/filters/local_ratelimit/config.h"

#include "envoy/registry/registry.h"
#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

FilterFactoryCb LocalRateLimitFilterConfig::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit& cfg, const std::string&,
    Server::Configuration::FactoryContext& context) {

  auto filter_config = std::make_shared<FilterConfig>(cfg, context.scope(), context.mainThreadDispatcher() );

  // cfg is changed
  return [cfg, filter_config](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(std::make_shared<LocalRateLimit>( filter_config ));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(LocalRateLimitFilterConfig, NamedMetaProtocolFilterConfigFactory);

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
