#pragma once

// Envoy
#include "envoy/local_info/local_info.h"
#include "envoy/server/factory_context.h"
#include "source/common/common/logger.h"

// istio proxy
#include "extensions/common/proto_util.h"

#include "api/meta_protocol_proxy/filters/metadata_exchange/v1alpha/metadata_exchange.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

const std::string ExchangeMetadataHeader = "x-envoy-peer-metadata";
const std::string ExchangeMetadataHeaderId = "x-envoy-peer-metadata-id";

class MetadataExchangeFilter : public CodecFilter, Logger::Loggable<Logger::Id::filter> {
public:
  MetadataExchangeFilter(
      const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange&,
      const Server::Configuration::FactoryContext& context);
  ~MetadataExchangeFilter() override = default;
  void onDestroy() override{};

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks&) override{};
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks&) override{};
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  // Helper function to get node metadata.
  void loadMetadataFromNodeInfo(const LocalInfo::LocalInfo& local_info);

  // base64 enocoded metadata
  std::string local_node_metadata_;
  // use node id as metadata id
  std::string local_node_metadata_id_;
  // traffic direction, inbound or outbound
  envoy::config::core::v3::TrafficDirection traffic_direction_;
};

} // namespace MetadataExchange
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
