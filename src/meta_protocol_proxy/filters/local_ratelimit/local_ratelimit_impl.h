#pragma once

#include <chrono>

#include "envoy/event/dispatcher.h"
#include "envoy/event/timer.h"
#include "envoy/extensions/common/ratelimit/v3/ratelimit.pb.h"
#include "envoy/ratelimit/ratelimit.h"

#include "source/common/common/thread_synchronizer.h"
#include "source/common/protobuf/protobuf.h"
#include "source/common/http/header_utility.h"
#include "src/meta_protocol_proxy/codec/codec.h"

#include "api/meta_protocol_proxy/filters/local_ratelimit/v1alpha/local_ratelimit.pb.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

using LocalRateLimitConfig =
aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit;

class LocalRateLimiterImpl {
public:
  LocalRateLimiterImpl(
      const std::chrono::milliseconds fill_interval, const uint32_t max_tokens,
      const uint32_t tokens_per_fill, Event::Dispatcher& dispatcher,
      const Protobuf::RepeatedPtrField<
          aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimitCondition>&
      conditions,
      const LocalRateLimitConfig& cfg);
  ~LocalRateLimiterImpl();

  bool requestAllowed(MetadataSharedPtr metadata) const;

private:
  struct TokenState {
    mutable std::atomic<uint32_t> tokens_;
    MonotonicTime fill_time_;
  };

  struct LocalRateLimitCondition {
    std::unique_ptr<TokenState> token_state_;
    std::vector<Http::HeaderUtility::HeaderDataPtr> match_;
    RateLimit::TokenBucket token_bucket_;
  };

  void onFillTimer();
  void onFillTimerHelper(const TokenState& state, const RateLimit::TokenBucket& bucket);
  void onFillTimerConditionHelper();
  bool requestAllowedHelper(const TokenState& tokens) const;

  RateLimit::TokenBucket global_token_bucket_; // The global token bucket for the whole service
  TokenState global_token_state_;                   // The global token for the whole service
  const Event::TimerPtr fill_timer_;
  TimeSource& time_source_;
  std::vector<LocalRateLimitCondition> conditions_;
  std::chrono::milliseconds timer_duration_;

  mutable Thread::ThreadSynchronizer synchronizer_; // Used for testing only.

  LocalRateLimitConfig config_;

  friend class LocalRateLimiterImplTest;
};

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
