#include "src/meta_protocol_proxy/filters/metadata_exchange/config.h"

#include "envoy/registry/registry.h"
#include "src/meta_protocol_proxy/filters/metadata_exchange/metadata_exchange.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

FilterFactoryCb MetadataExchangeFilterConfig::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange& cfg,
    const std::string&, Server::Configuration::FactoryContext& context) {

  // cfg is changed
  return [cfg, &context](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(std::make_shared<MetadataExchangeFilter>(context.clusterManager(), cfg));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(MetadataExchangeFilterConfig, NamedMetaProtocolFilterConfigFactory);

} // namespace MetadataExchange
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
