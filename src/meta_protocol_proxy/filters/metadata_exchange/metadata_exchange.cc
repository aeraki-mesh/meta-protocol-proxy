#include "src/meta_protocol_proxy/filters/metadata_exchange/metadata_exchange.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "source/common/buffer/buffer_impl.h"

#include "src/meta_protocol_proxy/filters/metadata_exchange/base64.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {
MetadataExchangeFilter::MetadataExchangeFilter(
    const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange&,
    const Server::Configuration::FactoryContext& context) {
  loadMetadataFromNodeInfo(context.local_info);
  traffic_direction_ = context.direction();
}

FilterStatus MetadataExchangeFilter::onMessageDecoded(MetadataSharedPtr,
                                                      MutationSharedPtr mutation) {
  if (traffic_direction_ == envoy::config::core::v3::TrafficDirection::OUTBOUND) {
    (*mutation.get())[ExchangeMetadataHeader] = metadata_;
    (*mutation.get())[ExchangeMetadataHeaderId] = metadata_id_;
  }
  return FilterStatus::ContinueIteration;
}

FilterStatus MetadataExchangeFilter::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::ContinueIteration;
}

void MetadataExchangeFilter::loadMetadataFromNodeInfo(const LocalInfo::LocalInfo& local_info) {
  if (local_info.node().has_metadata()) {
    google::protobuf::Struct metadata;
    const auto fb = ::Wasm::Common::extractNodeFlatBufferFromStruct(local_info.node().metadata());
    ::Wasm::Common::extractStructFromNodeFlatBuffer(
        *flatbuffers::GetRoot<::Wasm::Common::FlatNode>(fb.data()), &metadata);
    std::string metadata_bytes;
    ::Wasm::Common::serializeToStringDeterministic(metadata, &metadata_bytes);
    metadata_ = Base64::encode(metadata_bytes.data(), metadata_bytes.size());
  }
  metadata_id_ = local_info.node().id();
}

} // namespace MetadataExchange
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
