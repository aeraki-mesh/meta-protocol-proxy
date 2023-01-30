#include "src/meta_protocol_proxy/filters/stats/stats.h"

#include "src/meta_protocol_proxy/filters/common/base64.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Stats {
StatsFilter::StatsFilter(const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats&,
                         const Server::Configuration::FactoryContext& context) {
  traffic_direction_ = context.direction();
  local_node_info_ = Wasm::Common::extractEmptyNodeFlatBuffer();
  peer_node_info_ = Wasm::Common::extractEmptyNodeFlatBuffer();
  if (context.localInfo().node().has_metadata()) {
    local_node_info_ =
        Wasm::Common::extractNodeFlatBufferFromStruct(context.localInfo().node().metadata());
  }
}

// Returns a string view stored in a flatbuffers string.
static inline std::string_view GetFromFbStringView(const flatbuffers::String* str) {
  return str ? std::string_view(str->c_str(), str->size()) : std::string_view();
}

FilterStatus StatsFilter::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  // if this is an inbound request, we can extract the peer node metadata from the request header
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::INBOUND) {
    peer_node_info_ = extractPeerNodeMetadata(metadata);
  }
  ENVOY_LOG(info, "xxxxx stats request: {}", metadata->getRequestId());
  return FilterStatus::ContinueIteration;
}

FilterStatus StatsFilter::onMessageEncoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  // if this is an outbound request, we can extract the peer node metadata from the response header
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::OUTBOUND) {
    peer_node_info_ = extractPeerNodeMetadata(metadata);
  }
  const auto& peer_node = *flatbuffers::GetRoot<Wasm::Common::FlatNode>(peer_node_info_.data());
  const auto& local_node = *flatbuffers::GetRoot<Wasm::Common::FlatNode>(local_node_info_.data());
  ENVOY_LOG(info, "xxxxx local node: {}", GetFromFbStringView(local_node.workload_name()));
  ENVOY_LOG(info, "xxxxx peer node: {}", GetFromFbStringView(peer_node.workload_name()));
  ENVOY_LOG(info, "xxxxx stats response: {}", metadata->getRequestId());
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

} // namespace Stats
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
