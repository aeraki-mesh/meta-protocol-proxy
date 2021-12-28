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

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

/**
 * All local rate limit stats. @see stats_macros.h
 */
#define ALL_LOCAL_RATE_LIMIT_STATS(COUNTER)                                                        \
  COUNTER(rate_limited)                                                                            \
  COUNTER(ok)

/**
 * Struct definition for all local rate limit stats. @see stats_macros.h
 */
struct LocalRateLimitStats {
  ALL_LOCAL_RATE_LIMIT_STATS(GENERATE_COUNTER_STRUCT)
};

class FilterConfig {
  friend class LocalRateLimit;

public:
  FilterConfig(const LocalRateLimitConfig& cfg, Stats::Scope& scope, Event::Dispatcher& dispatcher);
  ~FilterConfig() = default;

private:
  LocalRateLimitStats generateStats(const std::string& prefix, Stats::Scope& scope);

  mutable LocalRateLimitStats stats_;
  LocalRateLimiterImpl rate_limiter_;
  LocalRateLimitConfig config_;
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

  LocalRateLimitStats generateStats(const std::string& prefix, Stats::Scope& scope);

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};

  std::shared_ptr<FilterConfig> filter_config_;
};

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

