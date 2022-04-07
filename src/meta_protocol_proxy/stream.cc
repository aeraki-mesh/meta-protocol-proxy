#include "src/meta_protocol_proxy/stream.h"
#include "src/meta_protocol_proxy/conn_manager.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

Stream::Stream(uint64_t stream_id, Network::Connection& downstream_conn,
               ConnectionManager& connection_manager)
    : stream_id_(stream_id), downstream_conn_(downstream_conn),
      connection_manager_(connection_manager){};

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
  if (end_stream) {
    connection_manager_.closeStream(stream_id_);
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
