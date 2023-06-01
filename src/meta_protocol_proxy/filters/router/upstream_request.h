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

class UpstreamRequestBase : public Logger::Loggable<Logger::Id::filter> {
public:
  UpstreamRequestBase(RequestOwner& parent, MetadataSharedPtr& metadata,
                      MutationSharedPtr& mutation);
  virtual ~UpstreamRequestBase() = default;

  virtual FilterStatus start() PURE;
  virtual void releaseUpStreamConnection(bool close) PURE;
  virtual void onRequestStart(bool continue_decoding);
  virtual void onRequestComplete() { request_complete_ = true; }
  virtual void onResponseStarted() { response_started_ = true; }
  virtual void onResponseComplete() { response_complete_ = true; }

  void onUpstreamConnectionEvent(Network::ConnectionEvent event);
  void onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host);
  void onUpstreamConnectionReset(ConnectionPool::PoolFailureReason reason);
  bool requestCompleted() { return request_complete_; };
  bool responseCompleted() { return response_complete_; };
  bool responseStarted() { return response_started_; };
  Upstream::HostDescriptionConstSharedPtr upstreamHost() { return upstream_host_; };

protected:
  RequestOwner& parent_;
  MetadataSharedPtr metadata_;
  MutationSharedPtr mutation_;

  Upstream::HostDescriptionConstSharedPtr upstream_host_;
  Envoy::Buffer::OwnedImpl upstream_request_buffer_;

  bool request_complete_ : 1;
  bool response_started_ : 1;
  bool response_complete_ : 1;
  bool stream_reset_ : 1;
};

class UpstreamRequest : public Tcp::ConnectionPool::Callbacks, public UpstreamRequestBase {
public:
  UpstreamRequest(RequestOwner& parent, Upstream::TcpPoolData& pool, MetadataSharedPtr& metadata,
                  MutationSharedPtr& mutation);
  virtual ~UpstreamRequest() {
    ENVOY_LOG(trace, "********** UpstreamRequest destructed ***********");
  };

  // Tcp::ConnectionPool::Callbacks
  void onPoolFailure(ConnectionPool::PoolFailureReason reason, absl::string_view,
                     Upstream::HostDescriptionConstSharedPtr host) override;
  void onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn,
                   Upstream::HostDescriptionConstSharedPtr host) override;

  // UpstreamRequestBase
  FilterStatus start() override;
  void releaseUpStreamConnection(const bool close) override;
  void onResponseComplete() override;

private:
  void encodeData(Buffer::Instance& data);

private:
  Upstream::TcpPoolData& conn_pool_;

  Tcp::ConnectionPool::Cancellable* conn_pool_handle_{};
  Tcp::ConnectionPool::ConnectionDataPtr conn_data_;
};

class UpstreamRequestByHandler : public UpstreamRequestCallbacks, public UpstreamRequestBase {
public:
  UpstreamRequestByHandler(RequestOwner& parent, MetadataSharedPtr& metadata,
                           MutationSharedPtr& mutation, UpstreamHandlerSharedPtr& upstream_handler);
  virtual ~UpstreamRequestByHandler() {
    ENVOY_LOG(trace, "********** UpstreamRequestByHandler destructed ***********");
  };

  // UpstreamRequestCallbacks
  void onPoolFailure(ConnectionPool::PoolFailureReason reason,
                     absl::string_view transport_failure_reason,
                     Upstream::HostDescriptionConstSharedPtr host) override;
  void onPoolReady(Upstream::HostDescriptionConstSharedPtr host) override;

  // UpstreamRequestBase
  FilterStatus start() override;
  void releaseUpStreamConnection(const bool close) override;

private:
  void encodeData(Buffer::Instance& data);

private:
  UpstreamHandlerSharedPtr upstream_handler_;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
