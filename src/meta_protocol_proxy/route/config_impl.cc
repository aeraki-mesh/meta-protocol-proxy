#include "src/meta_protocol_proxy/route/config_impl.h"

#include <memory>

#include "envoy/config/core/v3/base.pb.h"

#include "src/meta_protocol_proxy/route/route_matcher.h"
#include "src/meta_protocol_proxy/route/route_matcher_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

ConfigImpl::ConfigImpl(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        config,
    Server::Configuration::ServerFactoryContext& context)
    : name_(config.name()) {
  route_matcher_ = std::make_unique<
      Envoy::Extensions::NetworkFilters::MetaProtocolProxy::Router::RouteMatcherImpl>(config,
                                                                                      context);
}

RouteConstSharedPtr ConfigImpl::route(const Metadata& metadata, uint64_t random_value) const {
  return route_matcher_->route(metadata, random_value);
}

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
