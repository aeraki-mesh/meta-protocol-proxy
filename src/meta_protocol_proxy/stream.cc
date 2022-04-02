#include "src/meta_protocol_proxy/stream.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

void Stream::send2upstream(Buffer::Instance& data) {
  if (up_stream_conn_data_ != nullptr) {
    up_stream_conn_data_->connection().write(data, false);
  } else {
    ENVOY_LOG(error, "meta protocol: no upstream connection for stream {}, can't send message",
              stream_id_);
  }
}

void Stream::send2downstream(Buffer::Instance& data) { downstream_conn_.write(data, false); }

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
