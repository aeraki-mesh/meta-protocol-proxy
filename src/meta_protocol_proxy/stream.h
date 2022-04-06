#pragma once

#include "envoy/network/connection.h"
#include "common/buffer/buffer_impl.h"
#include "common/common/logger.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

class ConnectionManager;

// Stream tracks a streaming RPC, multiple requests and responses can be sent inside a stream.
class Stream : Tcp::ConnectionPool::UpstreamCallbacks,
               Event::DeferredDeletable,
               Logger::Loggable<Logger::Id::filter> {
public:
  Stream(uint64_t stream_id, Network::Connection& downstream_conn,
         ConnectionManager connection_manager)
      : stream_id_(stream_id), downstream_conn_(downstream_conn),
        connection_manager_(connection_manager){};
  ~Stream() = default;

  // UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override {
    send2downstream(data, end_stream);
  } // todo: we need to close the stream
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  void send2upstream(Buffer::Instance& data);
  void send2downstream(Buffer::Instance& data, bool end_stream);
  void setUpstreamConn(Tcp::ConnectionPool::ConnectionDataPtr upstream_conn_data);
  void halfClose() { half_close_ = true; }

private:
  uint64_t stream_id_;
  Tcp::ConnectionPool::ConnectionDataPtr upstream_conn_data_;
  Network::Connection& downstream_conn_;
  ConnectionManager& connection_manager_;
  bool half_close_{false};
};

using StreamPtr = std::unique_ptr<Stream>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
