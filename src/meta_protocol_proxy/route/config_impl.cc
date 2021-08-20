#include "src/meta_protocol_proxy/route/config_impl.h"

#include <memory>

#include "src/meta_protocol_proxy/route/route_matcher.h"
#include "src/meta_protocol_proxy/route/route_matcher_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

ConfigImpl::ConfigImpl(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        config,
    Server::Configuration::ServerFactoryContext& context)
    : name_(config.name()) {
  route_matcher_ = std::make_unique<RouteMatcherImpl>(config, context);
}

RouteConstSharedPtr ConfigImpl::route(const Metadata& metadata, uint64_t random_value) const {
  return route_matcher_->route(metadata, random_value);
}

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
