#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_manager.h"

#include "common/buffer/buffer_impl.h"
#include "envoy/extensions/filters/network/local_ratelimit/v3/local_rate_limit.pb.h"
#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

LocalRateLimitManager::LocalRateLimitManager(Stats::Scope& scope, const LocalRateLimitConfig& config,Event::Dispatcher& dispatcher)
        :config_(config) {

  // create ratelimit impl
  for (auto item : config.items()) { 
    std::string final_prefix = "local_rate_limit." + item.stat_prefix();

    // 禁止重复的 stat_prefix
    if (rate_limiter_map_.find(final_prefix) != rate_limiter_map_.end()) {
      throw EnvoyException("local rate limit config: stat_prefix already exists! prefix=" + item.stat_prefix());
    }

    // limiter map
    rate_limiter_map_[final_prefix] = new LocalRateLimiterImpl(
      std::chrono::milliseconds(PROTOBUF_GET_MS_REQUIRED(item.token_bucket(), fill_interval)),
      item.token_bucket().max_tokens(),
      PROTOBUF_GET_WRAPPED_OR_DEFAULT(item.token_bucket(), tokens_per_fill, 1),
      dispatcher,
      item
    );
    
    // stats
    stats_map_[final_prefix] = new LocalRateLimitStats{
      ALL_LOCAL_RATE_LIMIT_STATS(POOL_COUNTER_PREFIX(scope, final_prefix))
    };
  }
}

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy