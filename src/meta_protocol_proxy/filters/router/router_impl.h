#pragma once

#include <memory>

#include "envoy/buffer/buffer.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/upstream/load_balancer_impl.h"

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/filters/router/upstream_request.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

class Router : public Tcp::ConnectionPool::UpstreamCallbacks,
               public Upstream::LoadBalancerContextBase,
               public RequestOwner,
               public CodecFilter,
               Logger::Loggable<Logger::Id::filter> {
public:
  Router(Upstream::ClusterManager& cluster_manager) : cluster_manager_(cluster_manager) {}
  ~Router() override = default;

  // DecoderFilter
  void onDestroy() override;
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;

  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  // EncoderFilter
  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  // Upstream::LoadBalancerContextBase
  absl::optional<uint64_t> computeHashKey() override;
  const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() override { return nullptr; }
  const Network::Connection* downstreamConnection() const override;

  // Tcp::ConnectionPool::UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override;
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // RequestOwner
  Tcp::ConnectionPool::UpstreamCallbacks& upstreamCallbacks() override;
  DecoderFilterCallbacks& decoderFilterCallbacks() override;
  EncoderFilterCallbacks& encoderFilterCallbacks() override;

  // This function is for testing only.
  // Envoy::Buffer::Instance& upstreamRequestBufferForTest() { return upstream_request_buffer_; }

private:
  void cleanUpstreamRequest();
  bool upstreamRequestFinished() { return upstream_request_ == nullptr; };

  Upstream::ClusterManager& cluster_manager_;

  DecoderFilterCallbacks* decoder_filter_callbacks_{};
  EncoderFilterCallbacks* encoder_filter_callbacks_{};
  Route::RouteConstSharedPtr route_{};
  const Route::RouteEntry* route_entry_{};
  Upstream::ClusterInfoConstSharedPtr cluster_;

  std::unique_ptr<UpstreamRequest> upstream_request_;
  MetadataSharedPtr requestMetadata_;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
