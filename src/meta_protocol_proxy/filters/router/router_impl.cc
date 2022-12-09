#include "src/meta_protocol_proxy/filters/router/router_impl.h"

#include "envoy/upstream/thread_local_cluster.h"
#include "envoy/tracing/trace_reason.h"

#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/codec_impl.h"
#include "src/meta_protocol_proxy/tracing/tracer_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

// ---- DecoderFilter ---- handle request path
void Router::onDestroy() {
  // close the upstream connection if the upstream request has not been completed because there may
  // be more data coming later on the connection after destroying the router
  if (!upstreamRequestFinished()) {
    upstream_request_->releaseUpStreamConnection(
        false); // todo: should be true, but we get segment fault in rare case
  }
  cleanUpstreamRequest();
}

void Router::setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) {
  decoder_filter_callbacks_ = &callbacks;
}

FilterStatus Router::onMessageDecoded(MetadataSharedPtr request_metadata,
                                      MutationSharedPtr request_mutation) {
  auto messageType = request_metadata->getMessageType();
  ASSERT(messageType == MessageType::Request || messageType == MessageType::Stream_Init);

  request_metadata_ = request_metadata;
  route_ = decoder_filter_callbacks_->route();
  if (!route_) {
    ENVOY_STREAM_LOG(debug, "meta protocol router: no cluster match for request '{}'",
                     *decoder_filter_callbacks_, request_metadata_->getRequestId());
    decoder_filter_callbacks_->sendLocalReply(
        AppException(Error{ErrorType::RouteNotFound,
                           fmt::format("meta protocol router: no cluster match for request '{}'",
                                       request_metadata_->getRequestId())}),
        false);
    return FilterStatus::AbortIteration;
  }

  route_entry_ = route_->routeEntry();
  decoder_filter_callbacks_->streamInfo().setRouteName(route_entry_->routeName());
  const std::string& cluster_name = route_entry_->clusterName();

  auto prepare_result = prepareUpstreamRequest(cluster_name, request_metadata_, this);
  if (prepare_result.exception.has_value()) {
    decoder_filter_callbacks_->sendLocalReply(prepare_result.exception.value(), false);
    return FilterStatus::AbortIteration;
  }
  auto& conn_pool_data = prepare_result.conn_pool_data.value();

  ENVOY_STREAM_LOG(debug, "meta protocol router: decoding request", *decoder_filter_callbacks_);

  // if x-request-id is created, then it's the first span in this trace
  is_first_span_ = setXRequestID(request_metadata, request_mutation);
  // only trace request if there's a tracing config
  if (decoder_filter_callbacks_->tracingConfig()) {
    traceRequest(request_metadata, request_mutation, cluster_name);
  }

  // Save the clone for request mirroring
  auto metadata_clone = request_metadata_->clone();

  route_entry_->requestMutation(request_mutation);
  upstream_request_ =
      std::make_unique<UpstreamRequest>(*this, conn_pool_data, request_metadata_, request_mutation);
  auto filter_status = upstream_request_->start();

  // Prepare connections for shadow routers, if there are mirror policies configured and currently
  // enabled.
  const auto& policies = route_entry_->requestMirrorPolicies();
  ENVOY_STREAM_LOG(debug, "meta protocol router: requestMirrorPolicies size:{}",
                   *decoder_filter_callbacks_, policies.size());

  if (!policies.empty()) {
    for (const auto& policy : policies) {
      if (policy->shouldShadow(runtime_, rand())) { // todo replace with rand generator of conn mgr
        // We can reuse the same metadata for each request because its original message will be
        // drained in the request
        ENVOY_STREAM_LOG(debug, "meta protocol router: mirror request size:{}",
                         *decoder_filter_callbacks_, metadata_clone->originMessage().length());
        shadow_writer_.submit(policy->clusterName(), metadata_clone->clone(), request_mutation,
                              *decoder_filter_callbacks_);
      }
    }
  }

  return filter_status;
}
// ---- DecoderFilter ----

// ---- EncoderFilter ---- handle response path
void Router::setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) {
  encoder_filter_callbacks_ = &callbacks;
}

FilterStatus Router::onMessageEncoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  if (upstream_request_ == nullptr) {
    return FilterStatus::ContinueIteration;
  }
  response_metadata_ = metadata;

  ENVOY_STREAM_LOG(trace, "meta protocol router: response status: {}", *encoder_filter_callbacks_,
                   static_cast<int>(metadata->getResponseStatus()));

  switch (metadata->getResponseStatus()) {
  case ResponseStatus::Ok:
    if (metadata->getMessageType() == MessageType::Error) {
      upstream_request_->upstreamHost()->outlierDetector().putResult(
          Upstream::Outlier::Result::ExtOriginRequestFailed);
    } else {
      upstream_request_->upstreamHost()->outlierDetector().putResult(
          Upstream::Outlier::Result::ExtOriginRequestSuccess);
    }
    break;
  case ResponseStatus::Error:
    upstream_request_->upstreamHost()->outlierDetector().putResult(
        Upstream::Outlier::Result::ExtOriginRequestFailed);
    break;
  default:
    break;
  }

  return FilterStatus::ContinueIteration;
}
// ---- EncoderFilter ---

// ---- Tcp::ConnectionPool::UpstreamCallbacks ----
void Router::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  // We shouldn't get more data after a response is completed, otherwise it's a codec issue
  ASSERT(!upstream_request_->responseCompleted());
  ENVOY_STREAM_LOG(debug, "meta protocol router: reading response: {} bytes",
                   *decoder_filter_callbacks_, data.length());

  // Start response when receiving the first packet
  if (!upstream_request_->responseStarted()) {
    decoder_filter_callbacks_->startUpstreamResponse(*request_metadata_);
    upstream_request_->onResponseStarted();
  }

  UpstreamResponseStatus status = decoder_filter_callbacks_->upstreamData(data);
  const MetadataImpl* requestMetadataImpl = static_cast<const MetadataImpl*>(&(*request_metadata_));
  const auto& requestHeaders = requestMetadataImpl->getHeaders();
  const MetadataImpl* responseMetadataImpl =
      static_cast<const MetadataImpl*>(&(*response_metadata_));
  const auto& responseHeaders = responseMetadataImpl->getResponseHeaders();
  switch (status) {
  case UpstreamResponseStatus::Complete:
    ENVOY_STREAM_LOG(debug, "meta protocol router: response complete", *decoder_filter_callbacks_);
    upstream_request_->onResponseComplete();
    cleanUpstreamRequest();
    if (active_span_) {
      assert(response_metadata_);
      Tracing::MetaProtocolTracerUtility::finalizeSpanWithResponse(
          *active_span_, *response_metadata_, decoder_filter_callbacks_->streamInfo(),
          *decoder_filter_callbacks_->tracingConfig());
      ENVOY_STREAM_LOG(debug, "meta protocol router: finish tracing span",
                       *decoder_filter_callbacks_);
    }

    for (const auto& access_log : decoder_filter_callbacks_->accessLogs()) {
      access_log->log(&requestHeaders, &responseHeaders, nullptr,
                      decoder_filter_callbacks_->streamInfo());
    }
    return;
  case UpstreamResponseStatus::Reset:
    ENVOY_STREAM_LOG(debug, "meta protocol router: upstream reset", *decoder_filter_callbacks_);
    // When the upstreamData function returns Reset,
    // the current stream is already released from the upper layer,
    // so there is no need to call callbacks_->resetStream() to notify
    // the upper layer to release the stream.
    upstream_request_->releaseUpStreamConnection(true);
    if (active_span_) {
      Tracing::MetaProtocolTracerUtility::finalizeSpanWithoutResponse(
          *active_span_, decoder_filter_callbacks_->streamInfo(),
          *decoder_filter_callbacks_->tracingConfig(), ResponseStatus::Error);
      ENVOY_STREAM_LOG(debug, "meta protocol router: finish tracing span",
                       *decoder_filter_callbacks_);
    }
    return;
  case UpstreamResponseStatus::MoreData:
    // Response is incomplete, but no more data is coming. Probably codec or application side error.
    if (end_stream) {
      ENVOY_STREAM_LOG(debug,
                       "meta protocol router: response is incomplete, but no more data is coming",
                       *decoder_filter_callbacks_);
      upstream_request_->onUpstreamConnectionReset(
          ConnectionPool::PoolFailureReason::RemoteConnectionFailure);
      upstream_request_->onResponseComplete();
      cleanUpstreamRequest();
      if (active_span_) {
        Tracing::MetaProtocolTracerUtility::finalizeSpanWithoutResponse(
            *active_span_, decoder_filter_callbacks_->streamInfo(),
            *decoder_filter_callbacks_->tracingConfig(), ResponseStatus::Error);
        ENVOY_STREAM_LOG(debug, "meta protocol router: finish tracing span",
                         *decoder_filter_callbacks_);
      }
      return;
      // todo we also need to clean the stream
    }
    return;
  default:
    PANIC("not reached");
  }
}

void Router::onEvent(Network::ConnectionEvent event) {
  ASSERT(upstream_request_);

  upstream_request_->onUpstreamConnectionEvent(event);
  if (active_span_) {
    Tracing::MetaProtocolTracerUtility::finalizeSpanWithoutResponse(
        *active_span_, decoder_filter_callbacks_->streamInfo(),
        *decoder_filter_callbacks_->tracingConfig(), ResponseStatus::Error);
    ENVOY_STREAM_LOG(debug, "meta protocol router: finish tracing span",
                     *decoder_filter_callbacks_);
  }
}
// ---- Tcp::ConnectionPool::UpstreamCallbacks ----

// ---- Upstream::LoadBalancerContextBase ----
absl::optional<uint64_t> Router::computeHashKey() {
  if (auto* hash_policy = route_entry_->hashPolicy(); hash_policy != nullptr) {
    auto hash = hash_policy->generateHash(*request_metadata_);
    if (hash.has_value()) {
      ENVOY_STREAM_LOG(debug, "meta protocol router: computeHashKey: {}",
                       *decoder_filter_callbacks_, hash.value());
    }
    return hash;
  }

  return {};
}

const Network::Connection* Router::downstreamConnection() const {
  return decoder_filter_callbacks_ != nullptr ? decoder_filter_callbacks_->connection() : nullptr;
}
// ---- Upstream::LoadBalancerContextBase ----

bool Router::setXRequestID(MetadataSharedPtr& request_metadata,
                           MutationSharedPtr& request_mutation) {
  auto rid_extension = decoder_filter_callbacks_->requestIDExtension();
  // set x-request-id to metadata, so it can be used to record tracing sampling decision
  // RequestIDExtension only sets x-request-id if it doesn't exist in the Metadata
  bool modified = rid_extension->set(*request_metadata, false);
  // add x-request-id to mutation, so it can be passed through to upstream requests
  if (modified) {
    (*request_mutation)[ReservedHeaders::RequestUUID] = rid_extension->get(*request_metadata);
  }
  return modified;
}

Envoy::Tracing::Reason Router::mutateTracingRequestMetadata(MetadataSharedPtr& request_metadata) {
  auto rid_extension = decoder_filter_callbacks_->requestIDExtension();
  Envoy::Tracing::Reason final_reason = Envoy::Tracing::Reason::NotTraceable;
  if (!rid_extension->useRequestIdForTraceSampling()) {
    return Envoy::Tracing::Reason::Sampling;
  }

  const auto rid_to_integer = rid_extension->toInteger(*request_metadata);
  // Skip if request-id is corrupted, or non-existent
  if (!rid_to_integer.has_value()) {
    ENVOY_STREAM_LOG(warn, "meta protocol router: corrupted x-request-id",
                     *decoder_filter_callbacks_);
    return final_reason;
  }
  const uint64_t result = rid_to_integer.value() % 10000;

  // We only need to make tracing decision on the first span. The following spans just follow the
  // tracing decision.
  final_reason = rid_extension->getTraceReason(*request_metadata);
  if (is_first_span_) {
    // std::string uuid = rid_extension->get(*request_metadata);
    auto client_sampling = decoder_filter_callbacks_->tracingConfig()->clientSampling();
    auto random_sampling = decoder_filter_callbacks_->tracingConfig()->randomSampling();
    auto overall_sampling = decoder_filter_callbacks_->tracingConfig()->overallSampling();

    bool hasClientTraceId = request_metadata->getString(ReservedHeaders::ClientTraceId) != "";
    bool envoyForceTrace = request_metadata->getString(ReservedHeaders::EnvoyForceTrace) != "";
    if (hasClientTraceId &&
        runtime_.snapshot().featureEnabled("tracing.client_enabled", client_sampling)) {
      ENVOY_STREAM_LOG(debug, "meta protocol router: trace reason: client forced",
                       *decoder_filter_callbacks_);
      final_reason = Envoy::Tracing::Reason::ClientForced;
      rid_extension->setTraceReason(*request_metadata, final_reason);
    } else if (envoyForceTrace) {
      ENVOY_STREAM_LOG(debug, "meta protocol router: trace reason: service forced",
                       *decoder_filter_callbacks_);
      final_reason = Envoy::Tracing::Reason::ServiceForced;
      rid_extension->setTraceReason(*request_metadata, final_reason);
    } else if (runtime_.snapshot().featureEnabled("tracing.random_sampling", random_sampling,
                                                  result)) {
      ENVOY_STREAM_LOG(debug, "meta protocol router: trace reason: random sampling",
                       *decoder_filter_callbacks_);
      final_reason = Envoy::Tracing::Reason::Sampling;
      rid_extension->setTraceReason(*request_metadata, final_reason);
    }

    if (final_reason != Envoy::Tracing::Reason::NotTraceable &&
        !runtime_.snapshot().featureEnabled("tracing.global_enabled", overall_sampling, result)) {
      final_reason = Envoy::Tracing::Reason::NotTraceable;
      rid_extension->setTraceReason(*request_metadata, final_reason);
    }
  }

  return final_reason;
}

void Router::traceRequest(MetadataSharedPtr request_metadata, MutationSharedPtr request_mutation,
                          const std::string& cluster_name) {
  Envoy::Tracing::Reason reason = mutateTracingRequestMetadata(request_metadata);
  if (reason == Envoy::Tracing::Reason::NotTraceable ||
      reason == Envoy::Tracing::Reason::HealthCheck) {
    return;
  }
  const Envoy::Tracing::Decision tracing_decision = Envoy::Tracing::Decision{reason, true};

  ENVOY_STREAM_LOG(debug, "meta protocol router: start tracing span", *decoder_filter_callbacks_);
  active_span_ = decoder_filter_callbacks_->tracer()->startSpan(
      *decoder_filter_callbacks_->tracingConfig(), *request_metadata, *request_mutation,
      decoder_filter_callbacks_->streamInfo(), cluster_name, tracing_decision);
}

void Router::resetStream() { decoder_filter_callbacks_->resetStream(); }

void Router::cleanUpstreamRequest() {
  ENVOY_STREAM_LOG(debug, "meta protocol router: clean upstream request",
                   *decoder_filter_callbacks_);
  if (upstream_request_) {
    upstream_request_.reset();
  }
};

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
