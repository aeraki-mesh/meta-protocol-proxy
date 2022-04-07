#include "src/meta_protocol_proxy/stream.h"
#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

Stream::Stream(uint64_t stream_id, Network::Connection& downstream_conn,
               ConnectionManager& connection_manager, Codec& codec)
    : stream_id_(stream_id), downstream_conn_(downstream_conn),
      connection_manager_(connection_manager), codec_(codec){};

void Stream::send2upstream(Buffer::Instance& data) {
  if (upstream_conn_data_ != nullptr) {
    upstream_conn_data_->connection().write(data, false);
  } else {
    ENVOY_LOG(error, "meta protocol: no upstream connection for stream {}, can't send message",
              stream_id_);
  }
  if (client_closed_ && server_closed_) {
    clear();
  }
}

void Stream::send2downstream(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(debug, "meta protocol: send upstream response to stream {}", stream_id_);
  auto metadata = std::make_unique<MetadataImpl>();
  DecodeStatus status = codec_.decode(data, *metadata);
  if (status == DecodeStatus::WaitForData) {
    ENVOY_LOG(debug, "meta protocol: response wait for data {}", stream_id_);
    return;
  }
  downstream_conn_.write(metadata->getOriginMessage(), end_stream);
  if (metadata->getMessageType() == MessageType::Stream_Close_one_way) {
    ENVOY_LOG(debug, "meta protocol: close server side stream {}", stream_id_);
    closeServerStream();
  }
  if (metadata->getMessageType() == MessageType::Stream_Close_two_way) {
    ENVOY_LOG(debug, "meta protocol: close the entire stream {}", stream_id_);
    closeClientStream();
    closeServerStream();
  }
  if (end_stream || (client_closed_ && server_closed_)) {
    clear();
  }
}

void Stream::clear() {
  ENVOY_LOG(debug, "meta protocol: close the entire stream {}", stream_id_);
  connection_manager_.closeStream(stream_id_);
  upstream_conn_data_->connection().removeConnectionCallbacks(*this);
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
