#pragma once

#include "envoy/network/connection.h"
#include "envoy/stats/timespan.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/logger.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

// Stream tracks a streaming RPC, multiple requests and responses can be sent inside a stream.
class Stream : Logger::Loggable<Logger::Id::filter> {
public:
  Stream(uint64_t stream_id, Network::Connection& downstream_conn)
      : stream_id_(stream_id), downstream_conn_(downstream_conn){};
  ~Stream() = default;

  void send2upstream(Buffer::Instance& data);
  void send2downstream(Buffer::Instance& data);

private:
  uint64_t stream_id_;
  Tcp::ConnectionPool::ConnectionDataPtr up_stream_conn_data_;
  Network::Connection& downstream_conn_;
};

using StreamPtr = std::unique_ptr<Stream>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
