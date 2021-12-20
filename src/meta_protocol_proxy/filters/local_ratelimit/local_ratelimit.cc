#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "common/buffer/buffer_impl.h"
#include "envoy/extensions/filters/network/local_ratelimit/v3/local_rate_limit.pb.h"
#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_impl.h"

#include "src/meta_protocol_proxy/filters/local_ratelimit/local_ratelimit_manager.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace LocalRateLimit {

void LocalRateLimit::onDestroy() {
  cleanup();
}

void LocalRateLimit::setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

FilterStatus LocalRateLimit::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {

  if (getLocalRateLimit(metadata)) {
    // 限流， 直接返回客户端
    ENVOY_STREAM_LOG(debug, "meta protocol ratelimit:  '{}'", *callbacks_, metadata->getRequestId());
    callbacks_->sendLocalReply(
        AppException(Error{ErrorType::OverLimit,
                           fmt::format("meta protocol router: no cluster match for request '{}'",
                                       metadata->getRequestId())}),
        false);
    return FilterStatus::StopIteration;
  }

  ENVOY_STREAM_LOG(debug, "meta protocol local ratelimit: decoding request", *callbacks_);
  return FilterStatus::Continue;
}

void LocalRateLimit::setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

FilterStatus LocalRateLimit::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::Continue;
}

void LocalRateLimit::cleanup() {}

// TODO
bool LocalRateLimit::getLocalRateLimit(MetadataSharedPtr metadata) {
  for (auto item : manager_->config_.items()) {
    std::string final_prefix = "local_rate_limit." + item.stat_prefix();

    bool isMatch = false;
    std::vector<RateLimit::LocalDescriptor> descriptors;
    RateLimit::LocalDescriptor descriptor;
    std::vector<RateLimit::DescriptorEntry> entries;

    for (auto match : item.match()) {
      // metadata: codec 解码，  match: 规则配置
      auto value = metadata->getString(match.key());
      // match， 规则为空 或者 规则值严格匹配 value
      if (value.size() > 0 && (match.value() == "" || match.value() == value)) {
        isMatch = true;
        RateLimit::DescriptorEntry entry;
        entry.key_ = match.key();
        entry.value_ = value;
        entries.emplace_back(entry);
      } else {
        isMatch = false;
        break;
      }
    }

    descriptor.entries_ = entries;

    descriptors.emplace_back(descriptor);

    if (isMatch) {
      manager_->stats_map_[final_prefix]->rate_limited_.inc();
      if(!manager_->rate_limiter_map_[final_prefix]->requestAllowed(descriptors)) {
          return true;
      }
    }
  }
  return false;
};


} // namespace LocalRateLimit
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy