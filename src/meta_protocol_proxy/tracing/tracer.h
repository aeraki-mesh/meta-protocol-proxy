#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "envoy/http/header_map.h"
#include "envoy/tracing/trace_driver.h"
#include "envoy/tracing/trace_reason.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

/**
 * MetaProtocolTracer is responsible for handling traces and delegate actions to the
 * corresponding drivers.
 */
class MetaProtocolTracer {
public:
  virtual ~MetaProtocolTracer() = default;

  virtual Envoy::Tracing::SpanPtr startSpan(const Envoy::Tracing::Config& config,
                                            Http::RequestHeaderMap& request_headers,
                                            const StreamInfo::StreamInfo& stream_info,
                                            const Envoy::Tracing::Decision tracing_decision) PURE;
};

using MetaProtocolTracerSharedPtr = std::shared_ptr<MetaProtocolTracer>;

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
