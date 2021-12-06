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
#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_impl.h"
#include "src/meta_protocol_proxy/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

/**
 * All local rate limit stats. @see stats_macros.h
 */
#define ALL_LOCAL_RATE_LIMIT_STATS(COUNTER) COUNTER(rate_limited)

/**
 * Struct definition for all local rate limit stats. @see stats_macros.h
 */
struct LocalRateLimitStats {
  ALL_LOCAL_RATE_LIMIT_STATS(GENERATE_COUNTER_STRUCT)
};

// using ConfigSharedPtr = std::shared_ptr<Envoy::Extensions::NetworkFilters::MetaProtocolProxy::LocalRateLimit::LocalRateLimiterImpl>;
using LocalRateLimitStatsPtr = std::shared_ptr<LocalRateLimitStats>;
using LocalRateLimitConfig = aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit;

class LocalRateLimitManager {
    friend class LocalRateLimit;
public:
  LocalRateLimitManager(Stats::Scope& scope, const LocalRateLimitConfig& config, Event::Dispatcher& dispatcher);
  ~LocalRateLimitManager() = default;

private:
  std::map<std::string, LocalRateLimiterImpl*> rate_limiter_map_;
  // std::map<std::string, LocalRateLimitStats*> stats_map_;
  LocalRateLimitConfig config_;
  std::vector<RateLimit::LocalDescriptor> descriptors_;
};

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
