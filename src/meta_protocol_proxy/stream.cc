#include "src/meta_protocol_proxy/stream.h"
#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/codec_impl.h"

#include "envoy/network/connection.h"

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
    ENVOY_LOG(debug, "meta protocol: send downstream request to stream {}", stream_id_);
    upstream_conn_data_->connection().write(data, false);
  } else {
    ENVOY_LOG(error, "meta protocol: no upstream connection for stream {}, can't send message",
              stream_id_);
  }
}

void Stream::send2downstream(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(debug, "meta protocol: send upstream response to stream {}", stream_id_);
  while (data.length() > 0) {
    auto metadata = std::make_unique<MetadataImpl>();
    metadata->setMessageType(MessageType::Response);
    DecodeStatus status = codec_.decode(data, *metadata);
    if (status == DecodeStatus::WaitForData) {
      ENVOY_LOG(debug, "meta protocol: response wait for data {}", stream_id_);
      return;
    }
    downstream_conn_.write(metadata->originMessage(), end_stream);
    if (metadata->getMessageType() == MessageType::Stream_Close_One_Way) {
      ENVOY_LOG(debug, "meta protocol: close server side stream {}", stream_id_);
      closeServerStream();
    }
    if (metadata->getMessageType() == MessageType::Stream_Close_Two_Way) {
      ENVOY_LOG(debug, "meta protocol: close the entire stream {}", stream_id_);
      closeClientStream();
      closeServerStream();
    }
    // According to tRPC protocol, a server close frame means the stream is closed.
    if (end_stream || (server_closed_)) {
      clear();
    }
  }
}

void Stream::clear() {
  ENVOY_LOG(debug, "meta protocol: close the entire stream {}", stream_id_);
  upstream_conn_data_->connection().removeConnectionCallbacks(*this);
  // In theory, we don't have to reset the unique prt, since it will be deleted automatically after
  // stream being deleted from the connection manager. Just do it for safety.
  upstream_conn_data_.reset();
  connection_manager_.closeStream(stream_id_);
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
