#include "src/meta_protocol_proxy/filters/istio_stats/istio_stats.h"
#include "src/meta_protocol_proxy/filters/istio_stats/stats_filter.h"

#include "src/meta_protocol_proxy/filters/common/base64.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {
StatsFilter::StatsFilter(const aeraki::meta_protocol_proxy::filters::istio_stats::v1alpha::IstioStats& config,
                         const Server::Configuration::FactoryContext& context,
                         IstioStats& istioStats)
    : istio_stats_(istioStats), destination_service_(config.destination_service()) {
  traffic_direction_ = context.direction();
  peer_node_info_ = Wasm::Common::extractEmptyNodeFlatBuffer();
}

FilterStatus StatsFilter::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  // if this is an inbound request, we can extract the peer node metadata from the request header
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::INBOUND) {
    peer_node_info_ = extractPeerNodeMetadata(metadata);
  }
  return FilterStatus::ContinueIteration;
}

FilterStatus StatsFilter::onMessageEncoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  // if this is an outbound request, we can extract the peer node metadata from the response header
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::OUTBOUND) {
    peer_node_info_ = extractPeerNodeMetadata(metadata);
  }
  const auto& peer_node = *flatbuffers::GetRoot<Wasm::Common::FlatNode>(peer_node_info_.data());
  istio_stats_.report(peer_node, metadata, destination_service_);
  return FilterStatus::ContinueIteration;
}

flatbuffers::DetachedBuffer StatsFilter::extractPeerNodeMetadata(MetadataSharedPtr metadata) {
  std::string metadataHeader = metadata->getString(ExchangeMetadataHeader);
  if (metadataHeader != "") {
    auto bytes = Base64::decodeWithoutPadding(metadataHeader);
    google::protobuf::Struct metadata;
    if (metadata.ParseFromString(bytes)) {
      return Wasm::Common::extractNodeFlatBufferFromStruct(metadata);
    }
  }
  return Wasm::Common::extractEmptyNodeFlatBuffer();
}

} // namespace IstioStats
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
