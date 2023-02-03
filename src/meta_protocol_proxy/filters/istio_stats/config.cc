#include "src/meta_protocol_proxy/filters/stats/config.h"

#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/filters/stats/stats_filter.h"
#include "src/meta_protocol_proxy/filters/stats/istio_stats.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {

FilterFactoryCb StatsFilterConfig::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats& cfg, const std::string&,
    Server::Configuration::FactoryContext& context) {
  auto stats = std::make_shared<IstioStats>(context.scope(), context.direction());
  // cfg is changed
  return [cfg, &context, &stats](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(std::make_shared<StatsFilter>(cfg, context, stats));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(StatsFilterConfig, NamedMetaProtocolFilterConfigFactory);

} // namespace IstioStats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
