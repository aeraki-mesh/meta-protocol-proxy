#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "envoy/tracing/trace_driver.h"
#include "envoy/tracing/trace_reason.h"

#include "src/meta_protocol_proxy/codec/codec.h"

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
                                            const Metadata& metadata,
                                            const StreamInfo::StreamInfo& stream_info,
                                            const Envoy::Tracing::Decision tracing_decision) PURE;
};

using MetaProtocolTracerSharedPtr = std::shared_ptr<MetaProtocolTracer>;

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
