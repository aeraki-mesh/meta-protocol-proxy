#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "envoy/local_info/local_info.h"
#include "envoy/stats/scope.h"
#include "envoy/buffer/buffer.h"
#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "envoy/network/connection.h"

#include "source/common/protobuf/protobuf.h"
#include "source/common/upstream/load_balancer_impl.h"
#include "source/common/http/header_utility.h"

#include "api/meta_protocol_proxy/filters/metadata_exchange/v1alpha/metadata_exchange.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "google/protobuf/util/json_util.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

class MetadataExchangeFilter : public CodecFilter,
                               public Upstream::LoadBalancerContextBase,
                               Logger::Loggable<Logger::Id::filter> {
public:
  MetadataExchangeFilter(
      const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange&,
      const LocalInfo::LocalInfo& local_info& local_info):local_info_(local_info) {}
  ~MetadataExchangeFilter() override = default;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& ) override{};
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& ) override {};
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  // Helper function to get node metadata.
  void getMetadata(google::protobuf::Struct* metadata);

  // Helper function to get metadata id.
  std::string getMetadataId();

  // LocalInfo instance.
  const LocalInfo::LocalInfo& local_info_;
};

} // namespace MetadataExchange
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
