#pragma once

#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.h"
#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.validate.h"
#include "api/meta_protocol_proxy/filters/router/v1alpha/router.pb.h"
#include "api/meta_protocol_proxy/filters/router/v1alpha/router.pb.validate.h"

#include "src/meta_protocol_proxy/filters/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

class RouterFilterConfig
    : public FactoryBase<envoy::extensions::filters::meta_protocol_proxy::router::v1alpha::Router> {
public:
  RouterFilterConfig() : FactoryBase("aeraki.meta_protocol.filters.router") {}

private:
  FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::meta_protocol_proxy::router::v1alpha::Router& proto_config,
      const std::string& stat_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
