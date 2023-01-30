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
}

// Returns a string view stored in a flatbuffers string.
static inline std::string_view GetFromFbStringView(const flatbuffers::String* str) {
  return str ? std::string_view(str->c_str(), str->size()) : std::string_view();
}

FilterStatus StatsFilter::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::INBOUND) {
    std::string metadataHeader = metadata->getString(ExchangeMetadataHeader);
    if (metadataHeader != "") {
      auto bytes = Base64::decodeWithoutPadding(metadataHeader);
      google::protobuf::Struct metadata;
      if (metadata.ParseFromString(bytes)) {
        auto fb = Wasm::Common::extractNodeFlatBufferFromStruct(metadata);
        const auto& local_node = *flatbuffers::GetRoot<Wasm::Common::FlatNode>(fb.data());
	ENVOY_LOG(info, "xxxxx {}", GetFromFbStringView(local_node.workload_name()));
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
