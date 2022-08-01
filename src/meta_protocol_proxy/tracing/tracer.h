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
                                            Metadata& metadata,
                                            const StreamInfo::StreamInfo& stream_info,
                                            const Envoy::Tracing::Decision tracing_decision) PURE;
};

using MetaProtocolTracerSharedPtr = std::shared_ptr<MetaProtocolTracer>;

/**
 * Configuration for tracing which is set on the MetaProtocol Proxy level.
 * Tracing can be enabled/disabled on a per MetaProtocol Proxy basis.
 * Here we specify some specific for MetaProtocol Proxy settings.
 */
class TracingConfig : public Envoy::Tracing::Config {
public:
  virtual ~TracingConfig() = default;
  virtual envoy::type::v3::FractionalPercent clientSampling() PURE;
  virtual envoy::type::v3::FractionalPercent randomSampling() PURE;
  virtual envoy::type::v3::FractionalPercent overallSampling() PURE;
};
using TracingConfigPtr = std::unique_ptr<TracingConfig>;

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
