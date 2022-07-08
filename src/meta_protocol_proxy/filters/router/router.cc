#include "src/meta_protocol_proxy/filters/router/router.h"

#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/codec/codec.h"

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
    upstream_request_->releaseUpStreamConnection(true);
  }
  cleanUpstreamRequest();
}

void Router::setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) {
  decoder_filter_callbacks_ = &callbacks;
}

FilterStatus Router::onMessageDecoded(MetadataSharedPtr metadata,
                                      MutationSharedPtr requestMutation) {
  auto messageType = metadata->getMessageType();
  ASSERT(messageType == MessageType::Request || messageType == MessageType::Stream_Init);

  requestMetadata_ = metadata;
  route_ = decoder_filter_callbacks_->route();
  if (!route_) {
    ENVOY_STREAM_LOG(debug, "meta protocol router: no cluster match for request '{}'",
                     *decoder_filter_callbacks_, metadata->getRequestId());
    decoder_filter_callbacks_->sendLocalReply(
        AppException(Error{ErrorType::RouteNotFound,
                           fmt::format("meta protocol router: no cluster match for request '{}'",
                                       metadata->getRequestId())}),
        false);
    return FilterStatus::AbortIteration;
  }

  route_entry_ = route_->routeEntry();

  Upstream::ThreadLocalCluster* cluster =
      cluster_manager_.getThreadLocalCluster(route_entry_->clusterName());
  if (!cluster) {
    ENVOY_STREAM_LOG(debug, "meta protocol router: unknown cluster '{}'",
                     *decoder_filter_callbacks_, route_entry_->clusterName());
    decoder_filter_callbacks_->sendLocalReply(
        AppException(Error{ErrorType::ClusterNotFound,
                           fmt::format("meta protocol router: unknown cluster '{}'",
                                       route_entry_->clusterName())}),
        false);
    return FilterStatus::AbortIteration;
  }

  cluster_ = cluster->info();
  ENVOY_STREAM_LOG(debug, "meta protocol router: cluster {} match for request '{}'",
                   *decoder_filter_callbacks_, cluster_->name(), metadata->getRequestId());

  if (cluster_->maintenanceMode()) {
    decoder_filter_callbacks_->sendLocalReply(
        AppException(Error{ErrorType::Unspecified,
                           fmt::format("meta protocol router: maintenance mode for cluster '{}'",
                                       route_entry_->clusterName())}),
        false);
    return FilterStatus::AbortIteration;
  }

  auto conn_pool = cluster->tcpConnPool(Upstream::ResourcePriority::Default, this);
  if (!conn_pool) {
    decoder_filter_callbacks_->sendLocalReply(
        AppException(Error{ErrorType::NoHealthyUpstream,
                           fmt::format("meta protocol router: no healthy upstream for '{}'",
                                       route_entry_->clusterName())}),
        false);
    return FilterStatus::AbortIteration;
  }

  ENVOY_STREAM_LOG(debug, "meta protocol router: decoding request", *decoder_filter_callbacks_);
  route_entry_->requestMutation(requestMutation);
  upstream_request_ =
      std::make_unique<UpstreamRequest>(*this, *conn_pool, requestMetadata_, requestMutation);
  return upstream_request_->start();
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

  ENVOY_STREAM_LOG(trace, "meta protocol router: response status: {}", *encoder_filter_callbacks_,
                   metadata->getResponseStatus());

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
    decoder_filter_callbacks_->startUpstreamResponse(*requestMetadata_);
    upstream_request_->onResponseStarted();
  }

  UpstreamResponseStatus status = decoder_filter_callbacks_->upstreamData(data);
  switch (status) {
  case UpstreamResponseStatus::Complete:
    ENVOY_STREAM_LOG(debug, "meta protocol router: response complete", *decoder_filter_callbacks_);
    upstream_request_->onResponseComplete();
    cleanUpstreamRequest();
    return;
  case UpstreamResponseStatus::Reset:
    ENVOY_STREAM_LOG(debug, "meta protocol router: upstream reset", *decoder_filter_callbacks_);
    // When the upstreamData function returns Reset,
    // the current stream is already released from the upper layer,
    // so there is no need to call callbacks_->resetStream() to notify
    // the upper layer to release the stream.
    upstream_request_->releaseUpStreamConnection(true);
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
      return;
      // todo we also need to clean the stream
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }
  }
}

void Router::onEvent(Network::ConnectionEvent event) {
  ASSERT(upstream_request_);

//  if (upstream_request_->stream_reset_ && event == Network::ConnectionEvent::LocalClose) {
//    ENVOY_LOG(debug, "meta protocol upstream request: the stream reset");
//    return;
//  }
  upstream_request_->onUpstreamConnectionEvent(event);
}
// ---- Tcp::ConnectionPool::UpstreamCallbacks ----

// ---- RequestOwner ----
Tcp::ConnectionPool::UpstreamCallbacks& Router::upstreamCallbacks() { return *this; }

DecoderFilterCallbacks& Router::decoderFilterCallbacks() { return *decoder_filter_callbacks_; }

EncoderFilterCallbacks& Router::encoderFilterCallbacks() { return *encoder_filter_callbacks_; }
// ---- RequestOwner ----

// ---- Upstream::LoadBalancerContextBase ----
absl::optional<uint64_t> Router::computeHashKey() {
  if (auto* hash_policy = route_entry_->hashPolicy(); hash_policy != nullptr) {
    auto hash = hash_policy->generateHash(*requestMetadata_);
    if (hash.has_value()) {
      ENVOY_LOG(debug, "meta protocol router: computeHashKey: {}", hash.value());
    }
    return hash;
  }

  return {};
}

const Network::Connection* Router::downstreamConnection() const {
  return decoder_filter_callbacks_ != nullptr ? decoder_filter_callbacks_->connection() : nullptr;
}
// ---- Upstream::LoadBalancerContextBase ----

void Router::cleanUpstreamRequest() {
  ENVOY_LOG(debug, "meta protocol router: clean upstream request");
  if (upstream_request_) {
    upstream_request_.reset();
  }
};

UpstreamRequest::UpstreamRequest(RequestOwner& parent, Upstream::TcpPoolData& pool,
                                 MetadataSharedPtr& metadata, MutationSharedPtr& mutation)
    : parent_(parent), conn_pool_(pool), metadata_(metadata), mutation_(mutation),
      request_complete_(false), response_started_(false), response_complete_(false),
      stream_reset_(false) {
  upstream_request_buffer_.move(metadata->getOriginMessage(),
                                metadata->getOriginMessage().length());
}

UpstreamRequest::~UpstreamRequest() = default;

FilterStatus UpstreamRequest::start() {
  Tcp::ConnectionPool::Cancellable* handle = conn_pool_.newConnection(*this);
  if (handle) {
    // Pause while we wait for a connection.
    conn_pool_handle_ = handle;
    return FilterStatus::PauseIteration;
  }

  return FilterStatus::ContinueIteration;
}

void UpstreamRequest::onUpstreamConnectionEvent(Network::ConnectionEvent event) {
  ASSERT(!response_complete_);

  switch (event) {
  case Network::ConnectionEvent::RemoteClose:
    ENVOY_LOG(debug, "meta protocol router: upstream remote close");
    onUpstreamConnectionReset(ConnectionPool::PoolFailureReason::RemoteConnectionFailure);
    upstream_host_->outlierDetector().putResult(
        Upstream::Outlier::Result::LocalOriginConnectFailed);
    break;
  case Network::ConnectionEvent::LocalClose:
    ENVOY_LOG(debug, "meta protocol router: upstream local close");
    onUpstreamConnectionReset(ConnectionPool::PoolFailureReason::LocalConnectionFailure);
    break;
  default:
    // Connected event is consumed by the connection pool.
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

void UpstreamRequest::releaseUpStreamConnection(bool close) {
  stream_reset_ = true;

  // we're still waiting for the connection pool to create an upstream connection
  if (conn_pool_handle_) {
    ASSERT(!conn_data_);
    conn_pool_handle_->cancel(Tcp::ConnectionPool::CancelPolicy::Default);
    conn_pool_handle_ = nullptr;
    ENVOY_LOG(debug, "meta protocol upstream request: cancel pending upstream connection");
  }

  // we already got an upstream connection from the pool
  if (conn_data_) {
    ASSERT(!conn_pool_handle_);

    // we shouldn't close the upstream connection unless explicitly asked at some exceptional cases
    if (close) {
      conn_data_->connection().close(Network::ConnectionCloseType::NoFlush);
      ENVOY_LOG(warn, "meta protocol upstream request: close upstream connection");
    }

    // upstream connection is released back to the pool for re-use when it's containing
    // ConnectionData is destroyed
    conn_data_.reset();
    ENVOY_LOG(debug, "meta protocol upstream request: release upstream connection");
  }
}

void UpstreamRequest::encodeData(Buffer::Instance& data) {
  ASSERT(conn_data_);
  ASSERT(!conn_pool_handle_);

  ENVOY_STREAM_LOG(trace, "proxying {} bytes", parent_.decoderFilterCallbacks(), data.length());
  auto codec = parent_.decoderFilterCallbacks().createCodec(); // TODO just create codec once
  codec->encode(*metadata_, *mutation_, data);
  conn_data_->connection().write(data, false);
}

void UpstreamRequest::onPoolFailure(ConnectionPool::PoolFailureReason reason, absl::string_view,
                                    Upstream::HostDescriptionConstSharedPtr host) {
  conn_pool_handle_ = nullptr;

  // Mimic an upstream reset.
  onUpstreamHostSelected(host);
  onUpstreamConnectionReset(reason);

  upstream_request_buffer_.drain(upstream_request_buffer_.length());

  // If it is a connection error, it means that the connection pool returned
  // the error asynchronously and the upper layer needs to be notified to continue decoding.
  // If it is a non-connection error, it is returned synchronously from the connection pool
  // and is still in the callback at the current Filter, nothing to do.
  if (reason == ConnectionPool::PoolFailureReason::Timeout ||
      reason == ConnectionPool::PoolFailureReason::LocalConnectionFailure ||
      reason == ConnectionPool::PoolFailureReason::RemoteConnectionFailure) {
    if (reason == ConnectionPool::PoolFailureReason::Timeout) {
      host->outlierDetector().putResult(Upstream::Outlier::Result::LocalOriginTimeout);
    } else if (reason == ConnectionPool::PoolFailureReason::RemoteConnectionFailure) {
      host->outlierDetector().putResult(Upstream::Outlier::Result::LocalOriginConnectFailed);
    }
    parent_.decoderFilterCallbacks().continueDecoding();
  }
}

void UpstreamRequest::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                                  Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(debug, "meta protocol upstream request: tcp connection has ready");

  // Only invoke continueDecoding if we'd previously stopped the filter chain.
  bool continue_decoding = conn_pool_handle_ != nullptr;

  onUpstreamHostSelected(host);
  host->outlierDetector().putResult(Upstream::Outlier::Result::LocalOriginConnectSuccess);

  conn_data_ = std::move(conn_data);
  if (metadata_->getMessageType() == MessageType::Request) {
    conn_data_->addUpstreamCallbacks(parent_.upstreamCallbacks());
  }
  conn_pool_handle_ = nullptr;

  // Store the upstream ip to the metadata, which will be used in the response
  metadata_->putString(
      Metadata::HEADER_REAL_SERVER_ADDRESS,
      conn_data_->connection().connectionInfoProvider().remoteAddress()->asString());

  onRequestStart(continue_decoding);
  encodeData(upstream_request_buffer_);

  if (metadata_->getMessageType() == MessageType::Stream_Init) {
    // For streaming requests, we handle the following server response message in the stream
    ENVOY_LOG(debug, "meta protocol upstream request: the request is a stream init message");
    // todo change to a more appreciate method name, maybe clearMessage()
    parent_.decoderFilterCallbacks().resetStream();
    parent_.decoderFilterCallbacks().setUpstreamConnection(std::move(conn_data_));
  }
}

void UpstreamRequest::onRequestStart(bool continue_decoding) {
  ENVOY_LOG(debug, "meta protocol upstream request: start sending data to the server {}",
            upstream_host_->address()->asString());

  if (continue_decoding) {
    parent_.decoderFilterCallbacks().continueDecoding();
  }
  onRequestComplete();
}

void UpstreamRequest::onRequestComplete() { request_complete_ = true; }

void UpstreamRequest::onResponseComplete() {
  response_complete_ = true;
  conn_data_.reset();
}

void UpstreamRequest::onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(debug, "meta protocol upstream request: selected upstream {}",
            host->address()->asString());
  upstream_host_ = host;
}

void UpstreamRequest::onUpstreamConnectionReset(ConnectionPool::PoolFailureReason reason) {
  if (metadata_->getMessageType() == MessageType::Oneway) {
    // For oneway requests, we should not attempt a response. Reset the downstream to signal
    // an error.
    ENVOY_LOG(debug,
              "meta protocol upstream request: the request is oneway, reset downstream stream");
    parent_.decoderFilterCallbacks().resetStream();
    return;
  }

  // When the filter's callback does not end, the sendLocalReply function call
  // triggers the release of the current stream at the end of the filter's callback.
  switch (reason) {
  case ConnectionPool::PoolFailureReason::Overflow:
    parent_.decoderFilterCallbacks().sendLocalReply(
        AppException(Error{ErrorType::Unspecified,
                           fmt::format("meta protocol upstream request: too many connections")}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::LocalConnectionFailure:
    // Should only happen if we closed the connection, due to an error condition, in which case
    // we've already handled any possible downstream response.
    parent_.decoderFilterCallbacks().sendLocalReply(
        AppException(
            Error{ErrorType::Unspecified,
                  fmt::format("meta protocol upstream request: local connection failure '{}'",
                              upstream_host_->address()->asString())}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
    parent_.decoderFilterCallbacks().sendLocalReply(
        AppException(
            Error{ErrorType::Unspecified,
                  fmt::format("meta protocol upstream request: remote connection failure '{}'",
                              upstream_host_->address()->asString())}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::Timeout:
    parent_.decoderFilterCallbacks().sendLocalReply(
        AppException(Error{
            ErrorType::Unspecified,
            fmt::format("meta protocol upstream request: connection failure '{}' due to timeout",
                        upstream_host_->address()->asString())}),
        false);
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  if (!response_complete_) {
    parent_.decoderFilterCallbacks().resetStream();
  }
}

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
