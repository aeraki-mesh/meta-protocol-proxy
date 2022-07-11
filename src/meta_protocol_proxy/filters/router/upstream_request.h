#pragma once

#include "envoy/buffer/buffer.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

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

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
