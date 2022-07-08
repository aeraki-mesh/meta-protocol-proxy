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

/**
 * This interface is used by an upstream request to communicate its state.
 */
class RequestOwner {
public:
   virtual ~RequestOwner() = default;
  /**
   * @return ConnectionPool::UpstreamCallbacks& the handler for upstream data.
   */
  virtual Tcp::ConnectionPool::UpstreamCallbacks& upstreamCallbacks() PURE;

  /*
   * @return DecoderFilterCallbacks
   */
  virtual DecoderFilterCallbacks& decoderFilterCallbacks() PURE;

  /*
   * @return EncoderFilterCallbacks
   */
  virtual EncoderFilterCallbacks& encoderFilterCallbacks() PURE;
};

class UpstreamRequest : public Tcp::ConnectionPool::Callbacks,
                        Logger::Loggable<Logger::Id::filter> {
public:
  UpstreamRequest(RequestOwner& parent, Upstream::TcpPoolData& pool, MetadataSharedPtr& metadata,
                  MutationSharedPtr& mutation);
  ~UpstreamRequest() override;

  // Tcp::ConnectionPool::Callbacks
  void onPoolFailure(ConnectionPool::PoolFailureReason reason, absl::string_view,
                     Upstream::HostDescriptionConstSharedPtr host) override;
  void onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn,
                   Upstream::HostDescriptionConstSharedPtr host) override;

  FilterStatus start();
  void onUpstreamConnectionEvent(Network::ConnectionEvent event);
  void releaseUpStreamConnection(const bool close);
  void encodeData(Buffer::Instance& data);
  void onRequestStart(bool continue_decoding);
  void onRequestComplete();
  void onResponseComplete();
  void onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host);
  void onUpstreamConnectionReset(ConnectionPool::PoolFailureReason reason);
  bool requestCompleted() { return request_complete_; };
  bool responseCompleted() { return response_complete_; };
  bool responseStarted() { return response_started_; };
  void onResponseStarted() { response_started_ = true; };
  Upstream::HostDescriptionConstSharedPtr upstreamHost() { return upstream_host_; };

private:
  RequestOwner& parent_;
  Upstream::TcpPoolData& conn_pool_;
  MetadataSharedPtr metadata_;
  MutationSharedPtr mutation_;

  Tcp::ConnectionPool::Cancellable* conn_pool_handle_{};
  Tcp::ConnectionPool::ConnectionDataPtr conn_data_;
  Upstream::HostDescriptionConstSharedPtr upstream_host_;
  Envoy::Buffer::OwnedImpl upstream_request_buffer_;

  bool request_complete_ : 1;
  bool response_started_ : 1;
  bool response_complete_ : 1;
  bool stream_reset_ : 1;
};

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
