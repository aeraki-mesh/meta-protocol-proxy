#include "src/meta_protocol_proxy/stream.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

void Stream::send2upstream(Buffer::Instance& data) {
  if (upstream_conn_data_ != nullptr) {
    upstream_conn_data_->connection().write(data, false);
  } else {
    ENVOY_LOG(error, "meta protocol: no upstream connection for stream {}, can't send message",
              stream_id_);
  }
}

void Stream::send2downstream(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(debug, "meta protocol: send upstream response to stream {}", stream_id_);
  downstream_conn_.write(data, end_stream);
  if (end_stream || half_close_) {
    connection_manager_.closeStream(metadata->getStreamId());
  }
}

void Stream::setUpstreamConn(Tcp::ConnectionPool::ConnectionDataPtr upstream_conn_data) {
  upstream_conn_data_ = std::move(upstream_conn_data);
  upstream_conn_data_->addUpstreamCallbacks(*this);
}

void Stream::onEvent(Network::ConnectionEvent) {
  // todo clean stream resource when connection has been closed
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
