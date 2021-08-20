#pragma once

#include <memory>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/upstream/load_balancer_impl.h"

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

class Router : public Tcp::ConnectionPool::UpstreamCallbacks,
               public Upstream::LoadBalancerContextBase,
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
  const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() override { return nullptr; }
  const Network::Connection* downstreamConnection() const override;

  // Tcp::ConnectionPool::UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override;
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // This function is for testing only.
  Envoy::Buffer::Instance& upstreamRequestBufferForTest() { return upstream_request_buffer_; }

private:
  struct UpstreamRequest : public Tcp::ConnectionPool::Callbacks {
    UpstreamRequest(Router& parent, Upstream::TcpPoolData& pool_data, MetadataSharedPtr& metadata);
    ~UpstreamRequest() override;

    FilterStatus start();
    void resetStream();
    void encodeData(Buffer::Instance& data);

    // Tcp::ConnectionPool::Callbacks
    void onPoolFailure(ConnectionPool::PoolFailureReason reason,
                       absl::string_view transport_failure_reason,
                       Upstream::HostDescriptionConstSharedPtr host) override;
    void onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn,
                     Upstream::HostDescriptionConstSharedPtr host) override;

    void onRequestStart(bool continue_decoding);
    void onRequestComplete();
    void onResponseComplete();
    void onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host);
    void onResetStream(ConnectionPool::PoolFailureReason reason);

    Router& parent_;
    Upstream::TcpPoolData conn_pool_data_;
    MetadataSharedPtr metadata_;
    MutationSharedPtr mutation_;

    Tcp::ConnectionPool::Cancellable* conn_pool_handle_{};
    Tcp::ConnectionPool::ConnectionDataPtr conn_data_;
    Upstream::HostDescriptionConstSharedPtr upstream_host_;

    bool request_complete_ : 1;
    bool response_started_ : 1;
    bool response_complete_ : 1;
    bool stream_reset_ : 1;
  };

  void cleanup();

  Upstream::ClusterManager& cluster_manager_;

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};
  Route::RouteConstSharedPtr route_{};
  const Route::RouteEntry* route_entry_{};
  Upstream::ClusterInfoConstSharedPtr cluster_;

  std::unique_ptr<UpstreamRequest> upstream_request_;
  Envoy::Buffer::OwnedImpl upstream_request_buffer_;

  bool filter_complete_{false};
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
