#pragma once

#include "envoy/event/deferred_deletable.h"
#include "envoy/network/connection.h"
#include "envoy/network/filter.h"
#include "envoy/stats/timespan.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/linked_object.h"
#include "common/common/logger.h"
#include "common/stream_info/stream_info_impl.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/decoder_event_handler.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/route/route.h"
#include "src/meta_protocol_proxy/stats.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

// Stream tracks a streaming RPC, multiple requests and responses can be sent inside a stream.
class Stream : Logger::Loggable<Logger::Id::filter> {
public:
  Stream() = default;
  ~Stream() = default;
};

using StreamPtr = std::unique_ptr<Stream>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
