#include "src/meta_protocol_proxy/filters/ratelimit/ratelimit.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "common/buffer/buffer_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace RateLimit {

void RateLimit::onDestroy() {
  cleanup();
}

void RateLimit::setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

FilterStatus RateLimit::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  // TODO 创建connection 后 关闭，待优化
  auto conn = cluster_manager_.getThreadLocalCluster(config_.rate_limit_service().grpc_service().envoy_grpc().cluster_name())->tcpConn(this);
  auto addr = conn.host_description_->address();
  conn.connection_->close(Envoy::Network::ConnectionCloseType::NoFlush);

  if (getRateLimit(addr->asString(), metadata)) {
    // 限流成功， 直接返回客户端
    ENVOY_STREAM_LOG(debug, "meta protocol ratelimit:  '{}'", *callbacks_, metadata->getRequestId());
    callbacks_->sendLocalReply(
        AppException(Error{ErrorType::OverLimit,
                           fmt::format("meta protocol router: no cluster match for request '{}'",
                                       metadata->getRequestId())}),
        false);
    return FilterStatus::StopIteration;
  }

  ENVOY_STREAM_LOG(debug, "meta protocol router: decoding request", *callbacks_);
  return FilterStatus::Continue;
}

void RateLimit::setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

FilterStatus RateLimit::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::Continue;
}

void RateLimit::cleanup() {

}

bool RateLimit::getRateLimit(const std::string& addr, MetadataSharedPtr metadata) {
  ::grpc::ClientContext ctx;
  ::envoy::service::ratelimit::v3::RateLimitRequest request;
  ::envoy::service::ratelimit::v3::RateLimitResponse response;

  for (auto item : config_.items()) {
    bool isMatch = false;
    request.set_domain(item.domain());
    // TODO 优化， 合并请求
    request.set_hits_addend(1);

    for (auto match : item.match()) {
      // metadata: codec 解码，  match: 规则配置
      auto value = metadata->getString(match.key());
      // match， 规则为空 或者 规则值严格匹配 value
      if (value.size() > 0 && (match.value() == "" || match.value() == value)) {
        isMatch = true;
        auto desc = request.add_descriptors();
        auto entry = desc->add_entries();
        entry->set_key(match.key());
        entry->set_value(value);
      } else {
        isMatch = false;
        break;
      }
    }

    if (!isMatch) {
      continue;
    }

    auto stub = envoy::service::ratelimit::v3::RateLimitService::NewStub(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials()));
    auto st =  stub->ShouldRateLimit(&ctx, request, &response);

    // std::cout << std::endl << std::endl;
    // std::cout << std::endl << std::endl;
    // std::cout << "ret:" << st.ok() << std::endl;
    // std::cout << "code:" << st.error_code() << std::endl;
    // std::cout << "msg:" << st.error_message() << std::endl;
    // std::cout << "detail:" << st.error_details() << std::endl;
    // std::cout << "rate limit ... " << std::endl;
    // std::cout << "rsp" << response << std::endl;
    // std::cout << "req" << request << std::endl;
    // std::cout << "cfg" << config_ << std::endl;
    // std::cout << std::endl << std::endl;
    // std::cout << std::endl << std::endl;

    // 请求 ratelimit 失败处理
    if (!st.ok() && item.failure_mode_deny()) {
      return true;
    }

    if (envoy::service::ratelimit::v3::RateLimitResponse_Code_OVER_LIMIT == response.overall_code()) {
      return true;
    }
  }

  return false;
}

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
