#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_impl.h"
#include "src/meta_protocol_proxy/codec_impl.h"

#include "common/protobuf/utility.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

LocalRateLimiterImpl::LocalRateLimiterImpl(
    const std::chrono::milliseconds fill_interval, const uint32_t max_tokens,
    const uint32_t tokens_per_fill, Event::Dispatcher& dispatcher,
    const Protobuf::RepeatedPtrField<
        aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimitCondition>&
    conditions,
    const LocalRateLimitConfig& cfg)
    : fill_timer_(dispatcher.createTimer([this] { onFillTimer(); })),
      time_source_(dispatcher.timeSource()), timer_duration_(fill_interval),
      config_(cfg){
  if (config_.has_token_bucket()) {
    // The global token bucket for the whole service
    global_token_bucket_.max_tokens_ = max_tokens;
    global_token_bucket_.tokens_per_fill_ = tokens_per_fill;
    global_token_bucket_.fill_interval_ = absl::FromChrono(fill_interval);
    global_token_state_.tokens_ = max_tokens;
  }

  // Use the minimum fill interval as the duration for the timer
  for (const auto& condition : conditions) {
    std::chrono::milliseconds condition_fill_interval =
        absl::ToChronoMilliseconds(absl::Milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(
                                                          condition.token_bucket(), fill_interval, 0)));
    if (timer_duration_ == std::chrono::milliseconds(0) || condition_fill_interval < timer_duration_) {
      timer_duration_ = condition_fill_interval;
    }
  }

  if (timer_duration_ < std::chrono::milliseconds(50)) {
    throw EnvoyException("local rate limit token bucket fill timer must be >= 50ms");
  }

  fill_timer_->enableTimer(timer_duration_);

  // The more specified rate limit conditions
  for (const auto& condition : conditions) {
    LocalRateLimitCondition new_condition;
    new_condition.match_ = Http::HeaderUtility::buildHeaderDataVector(condition.match().metadata());
    RateLimit::TokenBucket token_bucket;
    token_bucket.fill_interval_ =
        absl::Milliseconds(PROTOBUF_GET_MS_OR_DEFAULT(condition.token_bucket(), fill_interval, 0));
    token_bucket.max_tokens_ = condition.token_bucket().max_tokens();
    token_bucket.tokens_per_fill_ =
        PROTOBUF_GET_WRAPPED_OR_DEFAULT(condition.token_bucket(), tokens_per_fill, 1);
    new_condition.token_bucket_ = token_bucket;

    auto token_state = std::make_unique<TokenState>();
    token_state->tokens_ = token_bucket.max_tokens_;
    token_state->fill_time_ = time_source_.monotonicTime();
    new_condition.token_state_ = std::move(token_state);

    conditions_.emplace_back(std::move(new_condition));
  }
}

LocalRateLimiterImpl::~LocalRateLimiterImpl() {
  fill_timer_->disableTimer();
}

void LocalRateLimiterImpl::onFillTimer() {
  onFillTimerConditionHelper();
  fill_timer_->enableTimer(timer_duration_);
}

void LocalRateLimiterImpl::onFillTimerHelper(const TokenState& tokens,
                                             const RateLimit::TokenBucket& bucket) {
  // Relaxed consistency is used for all operations because we don't care about ordering, just the
  // final atomic correctness.
  uint32_t expected_tokens = tokens.tokens_.load(std::memory_order_relaxed);
  uint32_t new_tokens_value;
  do {
    // expected_tokens is either initialized above or reloaded during the CAS failure below.
    new_tokens_value = std::min(bucket.max_tokens_, expected_tokens + bucket.tokens_per_fill_);

    // Testing hook.
    synchronizer_.syncPoint("on_fill_timer_pre_cas");

    // Loop while the weak CAS fails trying to update the tokens value.
  } while (!tokens.tokens_.compare_exchange_weak(expected_tokens, new_tokens_value,
                                                 std::memory_order_relaxed));
}

void LocalRateLimiterImpl::onFillTimerConditionHelper() {
  auto current_time = time_source_.monotonicTime();

  if (config_.has_token_bucket()) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                                  global_token_state_.fill_time_) >=
        absl::ToChronoMilliseconds(global_token_bucket_.fill_interval_)) {
      onFillTimerHelper(global_token_state_, global_token_bucket_);
      global_token_state_.fill_time_ = current_time;
    }
  }

  for (const auto& condition : conditions_) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time -
                                                              condition.token_state_->fill_time_) >=
        absl::ToChronoMilliseconds(condition.token_bucket_.fill_interval_)) {
      onFillTimerHelper(*condition.token_state_, condition.token_bucket_);
      condition.token_state_->fill_time_ = current_time;
    }
  }
}

bool LocalRateLimiterImpl::requestAllowedHelper(const TokenState& tokens) const {
  // Relaxed consistency is used for all operations because we don't care about ordering, just the
  // final atomic correctness.
  uint32_t expected_tokens = tokens.tokens_.load(std::memory_order_relaxed);
  do {
    // expected_tokens is either initialized above or reloaded during the CAS failure below.
    if (expected_tokens == 0) {
      return false;
    }

    // Testing hook.
    synchronizer_.syncPoint("allowed_pre_cas");

    // Loop while the weak CAS fails trying to subtract 1 from expected.
  } while (!tokens.tokens_.compare_exchange_weak(expected_tokens, expected_tokens - 1,
                                                 std::memory_order_relaxed));

  // We successfully decremented the counter by 1.
  return true;
}

bool LocalRateLimiterImpl::requestAllowed(MetadataSharedPtr metadata) const {
  const MetadataImpl* metadataImpl = static_cast<const MetadataImpl*>(&(*metadata));
  const auto& headers = metadataImpl->getHeaders();

  // The more specific rate limit conditions are the first priority
  for (const auto& condition : conditions_) {
    if (Http::HeaderUtility::matchHeaders(headers, condition.match_)) {
      return requestAllowedHelper(*condition.token_state_);
    }
  }

  // Allow the requests if no global token bucket
  if (!config_.has_token_bucket()) {
    return true;
  }

  // Use the global token bucket as the fallback rate limit policy
  return requestAllowedHelper(global_token_state_);
}

} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
