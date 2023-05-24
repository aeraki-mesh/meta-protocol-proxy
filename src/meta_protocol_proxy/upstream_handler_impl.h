#pragma once

#include <memory>
#include <map>

#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"
#include "envoy/upstream/load_balancer.h"
#include "source/common/buffer/buffer_impl.h"
#include "src/meta_protocol_proxy/upstream_response.h"
#include "src/meta_protocol_proxy/upstream_handler.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
class UpstreamHandlerImpl : public UpstreamHandler,
                            public Tcp::ConnectionPool::Callbacks,
                            public Tcp::ConnectionPool::UpstreamCallbacks,
                            public MessageHandler,
                            Logger::Loggable<Logger::Id::filter> {
public:
  using DeleteCallbackType = std::function<void(const std::string&)>;
  UpstreamHandlerImpl(const std::string& key, Network::Connection& connection, Config& config,
                      DeleteCallbackType delete_callback)
      : key_(key), downstream_connection_(connection), config_(config),
        delete_callback_(delete_callback) {}
  ~UpstreamHandlerImpl() override;

  // UpstreamHandler
  int start(Upstream::TcpPoolData& pool_data) override;
  void onData(Buffer::Instance& data, bool end_stream) override;
  void addResponseCallback(uint64_t request_id, ResponseCallback callback) override;
  bool isPoolReady() override;
  void addUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) override;
  void removeUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) override;

  // Tcp::ConnectionPool::Callbacks
  void onPoolFailure(ConnectionPool::PoolFailureReason reason,
                     absl::string_view transport_failure_reason,
                     Upstream::HostDescriptionConstSharedPtr host) override;
  void onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                   Upstream::HostDescriptionConstSharedPtr host) override;

  // Tcp::ConnectionPool::UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override;
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // MessageHandler
  void onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

private:
  void onClose();

private:
  std::string key_;
  Network::Connection& downstream_connection_;
  Config& config_;
  DeleteCallbackType delete_callback_;
  Tcp::ConnectionPool::Cancellable* upstream_handle_{};
  Tcp::ConnectionPool::ConnectionDataPtr conn_data_;
  Upstream::HostDescriptionConstSharedPtr upstream_host_;

  std::vector<UpstreamRequestCallbacks*> upstream_request_callbacks_;

  // key: request id
  std::map<uint64_t, ResponseCallback> response_callbacks_;

  UpstreamResponsePtr upstream_response_;

  bool pool_ready_{false};
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
