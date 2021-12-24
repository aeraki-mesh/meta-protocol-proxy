#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "envoy/stats/scope.h"
#include "envoy/buffer/buffer.h"
#include "common/common/logger.h"
#include "common/buffer/buffer_impl.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"
#include "common/upstream/load_balancer_impl.h"
#include "envoy/network/connection.h"
#include "common/http/header_utility.h"

#include "api/meta_protocol_proxy/filters/global_ratelimit/v1alpha/global_ratelimit.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "envoy/service/ratelimit/v3/rls.grpc.pb.h"
#include "google/protobuf/util/json_util.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace RateLimit {

class RateLimit : public CodecFilter, public Upstream::LoadBalancerContextBase, Logger::Loggable<Logger::Id::filter> {
public:
  RateLimit(Envoy::Upstream::ClusterManager& cm, const aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit& config)
   : config_(config), cluster_manager_(cm), config_headers_(Http::HeaderUtility::buildHeaderDataVector(config.match().metadata())) {}
  ~RateLimit() override = default;

  void onDestroy() override;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:

  void cleanup();

  bool shouldRateLimit(const std::string& addr, MetadataSharedPtr metadata);

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};

  aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit config_;
  Upstream::ClusterManager& cluster_manager_;

  const std::vector<Http::HeaderUtility::HeaderDataPtr> config_headers_;
};


} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
