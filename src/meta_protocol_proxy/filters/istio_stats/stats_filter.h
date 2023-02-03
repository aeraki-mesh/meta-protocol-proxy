#pragma once

// Envoy
#include "envoy/local_info/local_info.h"
#include "envoy/server/factory_context.h"
#include "source/common/common/logger.h"

// istio proxy
#include "extensions/common/proto_util.h"
#include "extensions/common/context.h"

#include "api/meta_protocol_proxy/filters/stats/v1alpha/stats.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/filters/istio_stats/istio_stats.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {

const std::string ExchangeMetadataHeader = "x-envoy-peer-metadata";
const std::string ExchangeMetadataHeaderId = "x-envoy-peer-metadata-id";

class StatsFilter : public CodecFilter, Logger::Loggable<Logger::Id::filter> {
public:
  StatsFilter(const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats&,
              const Server::Configuration::FactoryContext& context);
  ~StatsFilter() override = default;
  void onDestroy() override{};

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks&) override{};
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks&) override{};
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  flatbuffers::DetachedBuffer extractPeerNodeMetadata(MetadataSharedPtr metadata);

  // traffic direction, inbound or outbound
  envoy::config::core::v3::TrafficDirection traffic_direction_;

  flatbuffers::DetachedBuffer local_node_info_;
  flatbuffers::DetachedBuffer peer_node_info_;
  IstioStats& istio_stats_;
};

} // namespace IstioStats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
