#pragma once

#include "api/meta_protocol_proxy/filters/metadata_exchange/v1alpha/metadata_exchange.pb.h"
#include "api/meta_protocol_proxy/filters/metadata_exchange/v1alpha/metadata_exchange.pb.validate.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

class MetadataExchangeFilterConfig
    : public FactoryBase<
          aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange> {
public:
  MetadataExchangeFilterConfig() : FactoryBase("aeraki.meta_protocol.filters.metadata_exchange") {}

private:
  FilterFactoryCb createFilterFactoryFromProtoTyped(
      const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange&
          proto_config,
      const std::string& stat_prefix, Server::Configuration::FactoryContext& context) override;
};

} // namespace MetadataExchange
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
