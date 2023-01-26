#pragma once

#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

// Envoy
#include "envoy/local_info/local_info.h"
#include "envoy/stats/scope.h"
#include "envoy/buffer/buffer.h"
#include "envoy/server/factory_context.h"
#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "envoy/network/connection.h"

#include "source/common/protobuf/protobuf.h"
#include "source/common/upstream/load_balancer_impl.h"
#include "source/common/http/header_utility.h"

// istio proxy
#include "extensions/common/proto_util.h"

#include "google/protobuf/util/json_util.h"

#include "api/meta_protocol_proxy/filters/stats/v1alpha/stats.pb.h"
#include "src/meta_protocol_proxy/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Stats {

const std::string ExchangeMetadataHeader = "x-envoy-peer-metadata";
const std::string ExchangeMetadataHeaderId = "x-envoy-peer-metadata-id";

class StatsFilter : public CodecFilter,
                    public Upstream::LoadBalancerContextBase,
                    Logger::Loggable<Logger::Id::filter> {
public:
  StatsFilter(const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats&,
              const Server::Configuration::FactoryContext& context);
  ~StatsFilter() override = default;
  void onDestroy() override{};

  // DecoderFilter
  void setDecoderFilterCallbacks(DecoderFilterCallbacks&) override{};
  FilterStatus onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  void setEncoderFilterCallbacks(EncoderFilterCallbacks&) override{};
  FilterStatus onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) override;

private:
  // traffic direction, inbound or outbound
  envoy::config::core::v3::TrafficDirection traffic_direction_;
};

} // namespace Stats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
