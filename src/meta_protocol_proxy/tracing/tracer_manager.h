#pragma once

#include "envoy/config/trace/v3/http_tracer.pb.h"
#include "src/meta_protocol_proxy/tracing/tracer.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

/**
 * An MetaProtocolTracer manager which ensures existence of at most one
 * MetaProtocolTracer instance for a given configuration.
 */
class MetaProtocolTracerManager {
public:
  virtual ~MetaProtocolTracerManager() = default;

  /**
   * Get an existing MetaProtocolTracer or create a new one for a given configuration.
   * @param config supplies the configuration for the tracing provider.
   * @return MetaProtocolTracerSharedPtr.
   */
  virtual MetaProtocolTracerSharedPtr
  getOrCreateMetaProtocolTracer(const envoy::config::trace::v3::Tracing_Http* config) PURE;
};

using MetaProtocolTracerManagerSharedPtr = std::shared_ptr<MetaProtocolTracerManager>;

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy