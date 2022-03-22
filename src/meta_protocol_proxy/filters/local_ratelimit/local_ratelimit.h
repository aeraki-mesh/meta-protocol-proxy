#pragma once

#include <memory>
#include <string>

#include "envoy/network/filter.h"
#include "envoy/stats/scope.h"
#include "common/common/logger.h"
#include "envoy/stats/stats_macros.h"
#include "envoy/ratelimit/ratelimit.h"
#include "envoy/event/timer.h"

#include "api/meta_protocol_proxy/filters/local_ratelimit/v1alpha/local_ratelimit.pb.h"

#include "src/meta_protocol_proxy/filters/filter.h"

#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_impl.h"
#include "src/meta_protocol_proxy/filters/local_ratelimit/stats.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

class FilterConfig {
public:
  FilterConfig(const LocalRateLimitConfig& cfg, Stats::Scope& scope, Event::Dispatcher& dispatcher);
  ~FilterConfig() = default;

  LocalRateLimitStats& stats() { return stats_; }
  LocalRateLimiterImpl& rateLimiter() { return rate_limiter_; }

private:
  LocalRateLimitStats stats_;
  LocalRateLimiterImpl rate_limiter_;
};

class LocalRateLimit : public CodecFilter, Logger::Loggable<Logger::Id::filter> {
public:
  LocalRateLimit(std::shared_ptr<FilterConfig> filter_config) : filter_config_(filter_config){};
  ~LocalRateLimit() override = default;

  void onDestroy() override;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  void cleanup();

  bool shouldRateLimit(MetadataSharedPtr metadata);

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};

  std::shared_ptr<FilterConfig> filter_config_;
};

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
