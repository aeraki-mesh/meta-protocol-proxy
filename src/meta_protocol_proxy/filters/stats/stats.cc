#include "src/meta_protocol_proxy/filters/stats/stats.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Stats {
StatsFilter::StatsFilter(const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats&,
                         const Server::Configuration::FactoryContext& context) {
  traffic_direction_ = context.direction();
}

FilterStatus StatsFilter::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::INBOUND) {
    std::string metadataHeader = metadata->getString(ExchangeMetadataHeader);
    if (metadataHeader != "") {
      auto bytes = Base64::decodeWithoutPadding(metadataHeader);
      google::protobuf::Struct metadata;
      if (metadata.ParseFromString(bytes)) {
        auto fb = ::Wasm::Common::extractNodeFlatBufferFromStruct(metadata);
        const auto& local_node = *flatbuffers::GetRoot<::Wasm::Common::FlatNode>(fb.data());
      }
    }
  }
  return FilterStatus::ContinueIteration;
}

FilterStatus StatsFilter::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::ContinueIteration;
}

} // namespace Stats
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
