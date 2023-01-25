#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "envoy/stats/scope.h"
#include "envoy/buffer/buffer.h"
#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"
#include "source/common/upstream/load_balancer_impl.h"
#include "envoy/network/connection.h"
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
  MetadataExchangeFilter(Envoy::Upstream::ClusterManager&,
            const aeraki::meta_protocol_proxy::filters::metadata_exchange::v1alpha::MetadataExchange&){}
  ~MetadataExchangeFilter() override = default;

  void onDestroy() override;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  void cleanup();

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};
};

} // namespace MetadataExchange
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
