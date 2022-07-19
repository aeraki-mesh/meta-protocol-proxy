#include "src/meta_protocol_proxy/filters/router/shadow_writer_impl.h"

#include "envoy/upstream/thread_local_cluster.h"

#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

void ShadowWriterImpl::submit(const std::string& cluster_name, MetadataSharedPtr request_metadata,
                              MutationSharedPtr request_mutation, Codec& codec) {
  auto shadow_router = std::make_unique<ShadowRouterImpl>(*this, cluster_name, request_metadata,
                                                          request_mutation, codec);
  ENVOY_LOG(debug, "meta protocol shadow router: send request to mirror host: {}", cluster_name);
  const bool created = shadow_router->createUpstreamRequest();
  if (!created) {
    return;
  }

  auto& active_routers = tls_->getTyped<ActiveRouters>().activeRouters();

  LinkedList::moveIntoList(std::move(shadow_router), active_routers);
  return;
}

ShadowRouterImpl::ShadowRouterImpl(ShadowWriterImpl& parent, const std::string& cluster_name,
                                   MetadataSharedPtr metadata, MutationSharedPtr mutation,
                                   Codec& codec)
    : RequestOwner(parent.clusterManager()), parent_(parent), cluster_name_(cluster_name),
      metadata_(metadata), mutation_(mutation), codec_(codec),
      decoder_(NullResponseDecoder(codec)) {}

bool ShadowRouterImpl::createUpstreamRequest() {
  auto prepare_result = prepareUpstreamRequest(cluster_name_, metadata_, this);
  if (prepare_result.exception.has_value()) {
    return false;
  }

  auto& conn_pool_data = prepare_result.conn_pool_data.value();

  upstream_request_ =
      std::make_unique<UpstreamRequest>(*this, conn_pool_data, metadata_, mutation_);
  upstream_request_->start();
  return true;
}

void ShadowRouterImpl::cleanup() {
  if (removed_) {
    return;
  }
  removed_ = true;

  upstream_request_->releaseUpStreamConnection(false);
  parent_.remove(*this);
}

bool ShadowRouterImpl::requestInProgress() { return !upstream_request_->requestCompleted(); }

// ---- Tcp::ConnectionPool::UpstreamCallbacks ----
void ShadowRouterImpl::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  // We shouldn't get more data after a response is completed, otherwise it's a codec issue
  ASSERT(!upstream_request_->responseCompleted());
  ENVOY_LOG(debug, "meta protocol shadow router: reading response: {} bytes", data.length());

  // Start response when receiving the first packet
  if (!upstream_request_->responseStarted()) {
    upstream_request_->onResponseStarted();
  }

  UpstreamResponseStatus status = decoder_.decode(data);
  ENVOY_LOG(debug, "******** meta protocol shadow router: response status {}", status);
  switch (status) {
  case UpstreamResponseStatus::Complete:
    ENVOY_LOG(debug, "meta protocol shadow router: response complete");
    upstream_request_->onResponseComplete();
    cleanup();
    return;
  case UpstreamResponseStatus::MoreData:
    // Response is incomplete, but no more data is coming. Probably codec or application side error.
    if (end_stream) {
      ENVOY_LOG(debug, "meta protocol router: response is incomplete, but no more data is coming");
      cleanup();
      return;
    }
    return;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

void ShadowRouterImpl::onEvent(Network::ConnectionEvent event) {
  upstream_request_->onUpstreamConnectionEvent(event);
  cleanup();
}

// ---- Tcp::ConnectionPool::UpstreamCallbacks ----

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
