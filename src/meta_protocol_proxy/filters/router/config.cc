#include "src/meta_protocol_proxy/filters/router/config.h"

#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/filters/router/router.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

FilterFactoryCb RouterFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::meta_protocol_proxy::router::v1alpha::Router&,
    const std::string&, Server::Configuration::FactoryContext& context) {
  return [&context](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(std::make_shared<Router>(context.clusterManager()));
  };
}

/**
 * Static registration for the router filter. @see RegisterFactory.
 */
REGISTER_FACTORY(RouterFilterConfig, NamedMetaProtocolFilterConfigFactory);

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
