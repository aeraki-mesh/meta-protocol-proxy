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
#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_manager.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

// /**
//  * All local rate limit stats. @see stats_macros.h
//  */
// #define ALL_LOCAL_RATE_LIMIT_STATS(COUNTER) COUNTER(rate_limited)

// /**
//  * Struct definition for all local rate limit stats. @see stats_macros.h
//  */
// struct LocalRateLimitStats {
//   ALL_LOCAL_RATE_LIMIT_STATS(GENERATE_COUNTER_STRUCT)
// };

// using ConfigSharedPtr = std::shared_ptr<Envoy::Extensions::NetworkFilters::MetaProtocolProxy::LocalRateLimit::LocalRateLimiterImpl>;

// using LocalRateLimitStatsPtr = std::shared_ptr<LocalRateLimitStats>;

class LocalRateLimit : public CodecFilter, Logger::Loggable<Logger::Id::filter> {
public:
  // LocalRateLimit(Stats::Scope& scope, 
  // const aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit& proto_config, 
  // Event::Dispatcher& dispatcher);
  LocalRateLimit(LocalRateLimitManager* manager) : manager_(manager) {};
  ~LocalRateLimit() override = default;

  void onDestroy() override;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:

  void cleanup();

  // void generateRateLimitMap(
  //     const aeraki::meta_protocol_proxy::filters::local_ratelimit::v1alpha::LocalRateLimit& config,
  //     Stats::Scope& scope, Event::Dispatcher& dispatcher);

  // LocalRateLimitStats generateStats(const std::string& prefix, Stats::Scope& scope);
  bool getLocalRateLimit(MetadataSharedPtr metadata);

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};

  LocalRateLimitManager* manager_{};
};


} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
