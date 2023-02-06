#pragma once

#include "api/meta_protocol_proxy/filters/istio_stats/v1alpha/istio_stats.pb.h"
#include "api/meta_protocol_proxy/filters/istio_stats/v1alpha/istio_stats.pb.validate.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {

class StatsFilterConfig
    : public FactoryBase<aeraki::meta_protocol_proxy::filters::istio_stats::v1alpha::IstioStats> {
public:
  StatsFilterConfig() : FactoryBase("aeraki.meta_protocol.filters.istio_stats") {}

private:
  FilterFactoryCb createFilterFactoryFromProtoTyped(
      const aeraki::meta_protocol_proxy::filters::istio_stats::v1alpha::IstioStats& proto_config,
      const std::string& stat_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace IstioStats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
