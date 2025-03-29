#include "src/meta_protocol_proxy/filters/router/config.h"

#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/filters/router/router_impl.h"
#include "src/meta_protocol_proxy/filters/router/shadow_writer_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

FilterFactoryCb RouterFilterConfig::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::filters::router::v1alpha::Router&, const std::string&,
    Server::Configuration::FactoryContext& context) {

  auto shadow_writer = std::make_shared<ShadowWriterImpl>(
      context.serverFactoryContext().clusterManager(), context.serverFactoryContext().mainThreadDispatcher(), context.serverFactoryContext().threadLocal());
  // This lambda captures the shadow_writer created above, thus shadowed requests won't be
  // destructed after the main request is finished.
  // The life span of shadow_writer is as long as the MetaProtocol ConfigImpl, see filter_factories_
  // member of the MetaProtocol ConfigImpl
  return [&context, shadow_writer](FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addFilter(
        std::make_shared<Router>(context.serverFactoryContext().clusterManager(), context.serverFactoryContext().runtime(), *shadow_writer));
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
