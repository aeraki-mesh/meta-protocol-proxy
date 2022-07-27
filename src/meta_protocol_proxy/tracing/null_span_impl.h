#pragma once

#include "envoy/tracing/trace_driver.h"

#include "source/common/common/empty_string.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

/**
 * Null implementation of Span.
 */
class NullSpan : public Envoy::Tracing::Span {
public:
  static NullSpan& instance() {
    static NullSpan* instance = new NullSpan();
    return *instance;
  }

  // Tracing::Span
  void setOperation(absl::string_view) override {}
  void setTag(absl::string_view, absl::string_view) override {}
  void log(SystemTime, const std::string&) override {}
  void finishSpan() override {}
  void injectContext(Envoy::Tracing::TraceContext&) override {}
  void setBaggage(absl::string_view, absl::string_view) override {}
  std::string getBaggage(absl::string_view) override { return EMPTY_STRING; }
  std::string getTraceIdAsHex() const override { return EMPTY_STRING; }
  Envoy::Tracing::SpanPtr spawnChild(const Envoy::Tracing::Config&, const std::string&,
                                     SystemTime) override {
    return Envoy::Tracing::SpanPtr{new NullSpan()};
  }
  void setSampled(bool) override {}
};

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
