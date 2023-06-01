#pragma once

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/stats.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/route/route.h"
#include "src/meta_protocol_proxy/route/rds.h"
#include "src/meta_protocol_proxy/tracing/tracer.h"
#include "src/meta_protocol_proxy/request_id/request_id_extension.h"


namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

/**
 * Config is a configuration interface for ConnectionManager.
 */
class Config {
public:
  virtual ~Config() = default;

  virtual FilterChainFactory& filterFactory() PURE;
  virtual MetaProtocolProxyStats& stats() PURE;
  virtual CodecPtr createCodec() PURE;
  virtual Route::Config& routerConfig() PURE;
  virtual std::string applicationProtocol() PURE;
  virtual absl::optional<std::chrono::milliseconds> idleTimeout() PURE;
  /**
   * @return Route::RouteConfigProvider* the configuration provider used to acquire a route
   *         config for each request flow. Pointer ownership is _not_ transferred to the caller of
   *         this function.
   */
  virtual Route::RouteConfigProvider* routeConfigProvider() PURE;
  virtual Tracing::MetaProtocolTracerSharedPtr tracer() PURE;
  virtual Tracing::TracingConfig* tracingConfig() PURE;
  virtual RequestIDExtensionSharedPtr requestIDExtension() PURE;
  virtual const std::vector<AccessLog::InstanceSharedPtr>& accessLogs() const PURE;
  virtual bool multiplexing() PURE;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
