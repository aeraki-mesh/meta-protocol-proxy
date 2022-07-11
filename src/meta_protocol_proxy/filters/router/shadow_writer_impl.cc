#include "src/meta_protocol_proxy/filters/router/shadow_writer_impl.h"

#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

absl::optional<std::reference_wrapper<ShadowRouterHandle>>
ShadowWriterImpl::submit(const std::string& cluster_name, MetadataSharedPtr request_metadata) {
  auto shadow_router = std::make_unique<ShadowRouterImpl>(*this, cluster_name, request_metadata);
  const bool created = shadow_router->createUpstreamRequest();
  if (!created) {
    stats_.named_.shadow_request_submit_failure_.inc();
    return absl::nullopt;
  }

  auto& active_routers = tls_->getTyped<ActiveRouters>().activeRouters();

  LinkedList::moveIntoList(std::move(shadow_router), active_routers);
  return *active_routers.front();
}

ShadowRouterImpl::ShadowRouterImpl(ShadowWriterImpl& parent, const std::string& cluster_name,
                                   MetadataSharedPtr& metadata)
    : RequestOwner(parent.clusterManager()), parent_(parent), cluster_name_(cluster_name),
      metadata_(metadata->clone()) {
  response_decoder_ = std::make_unique<NullResponseDecoder>(*transport_, *protocol_);
  upstream_response_callbacks_ =
      std::make_unique<ShadowUpstreamResponseCallbacksImpl>(*response_decoder_);
}

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
