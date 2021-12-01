#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "envoy/stats/scope.h"
#include "envoy/buffer/buffer.h"
#include "common/common/logger.h"
#include "common/buffer/buffer_impl.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"
#include "common/upstream/load_balancer_impl.h"
#include "envoy/network/connection.h"

#include "api/meta_protocol_proxy/filters/ratelimit/v1alpha/rls.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "envoy/service/ratelimit/v3/rls.grpc.pb.h"
#include "google/protobuf/util/json_util.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace RateLimit {

inline std::ostream& operator<<(std::ostream& out, const google::protobuf::Message& message) {
  std::string tmp;
  google::protobuf::util::JsonPrintOptions opts;

  // 是否把枚举值当作字符整形数,缺省是字符串
  opts.always_print_enums_as_ints = true;
  // 是否把下划线字段更改为驼峰格式,缺省时更改
  opts.preserve_proto_field_names = true;
  // 是否输出仅有默认值的原始字段,缺省时忽略
  opts.always_print_primitive_fields = true;

  google::protobuf::util::Status status =
      google::protobuf::util::MessageToJsonString(message, &tmp, opts);

  out << tmp;
  return out;
}

class RateLimit : public CodecFilter, public Upstream::LoadBalancerContextBase, Logger::Loggable<Logger::Id::filter> {
public:
  RateLimit(Envoy::Upstream::ClusterManager& cm, const aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit& config)
   : config_(config), cluster_manager_(cm) {


  }
  ~RateLimit() override = default;

  void onDestroy() override;

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) override;
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:

  void cleanup();

  bool getRateLimit(const std::string& addr, MetadataSharedPtr metadata);

  DecoderFilterCallbacks* callbacks_{};
  EncoderFilterCallbacks* encoder_callbacks_{};

  aeraki::meta_protocol_proxy::filters::ratelimit::v1alpha::RateLimit config_;
  Upstream::ClusterManager& cluster_manager_;
};


} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
