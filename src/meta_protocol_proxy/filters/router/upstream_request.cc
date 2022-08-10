#include "src/meta_protocol_proxy/filters/router/upstream_request.h"

#include "envoy/upstream/thread_local_cluster.h"

#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

UpstreamRequest::UpstreamRequest(RequestOwner& parent, Upstream::TcpPoolData& pool,
                                 MetadataSharedPtr& metadata, MutationSharedPtr& mutation)
    : parent_(parent), conn_pool_(pool), metadata_(metadata), mutation_(mutation),
      request_complete_(false), response_started_(false), response_complete_(false),
      stream_reset_(false) {
  upstream_request_buffer_.move(metadata->originMessage(), metadata->originMessage().length());
}

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

  ENVOY_LOG(trace, "proxying {} bytes", data.length());
  Codec& codec = parent_.codec();
  codec.encode(*metadata_, *mutation_, data);
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
    parent_.continueDecoding();
  }
}

void UpstreamRequest::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                                  Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(debug, "meta protocol upstream request: tcp connection is ready");

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
    parent_.resetStream();
    parent_.setUpstreamConnection(std::move(conn_data_));
  }
  request_complete_ = true;
}

void UpstreamRequest::onRequestStart(bool continue_decoding) {
  ENVOY_LOG(debug, "meta protocol upstream request: start sending data to the server {}",
            upstream_host_->address()->asString());

  if (continue_decoding) {
    parent_.continueDecoding();
  }
}

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
    parent_.resetStream();
    return;
  }

  // When the filter's callback does not end, the sendLocalReply function call
  // triggers the release of the current stream at the end of the filter's callback.
  switch (reason) {
  case ConnectionPool::PoolFailureReason::Overflow:
    parent_.sendLocalReply(
        AppException(Error{ErrorType::Unspecified,
                           fmt::format("meta protocol upstream request: too many connections")}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::LocalConnectionFailure:
    // Should only happen if we closed the connection, due to an error condition, in which case
    // we've already handled any possible downstream response.
    parent_.sendLocalReply(
        AppException(
            Error{ErrorType::Unspecified,
                  fmt::format("meta protocol upstream request: local connection failure '{}'",
                              upstream_host_->address()->asString())}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::RemoteConnectionFailure:
    parent_.sendLocalReply(
        AppException(
            Error{ErrorType::Unspecified,
                  fmt::format("meta protocol upstream request: remote connection failure '{}'",
                              upstream_host_->address()->asString())}),
        false);
    break;
  case ConnectionPool::PoolFailureReason::Timeout:
    parent_.sendLocalReply(
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
    parent_.resetStream();
  }
}

} // namespace Router
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
