#include "src/meta_protocol_proxy/upstream_handler_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

UpstreamHandlerImpl::~UpstreamHandlerImpl() {
  ENVOY_LOG(trace, "********** UpstreamHandlerImpl destructed ***********");
  if (upstream_handle_ != nullptr) {
    upstream_handle_->cancel(ConnectionPool::CancelPolicy::CloseExcess);
    upstream_handle_ = nullptr;
    ENVOY_LOG(debug, "UpstreamHandlerImpl: reset connection pool handler");
  }
}

void UpstreamHandlerImpl::onClose() {
  ENVOY_LOG(debug, "UpstreamHandlerImpl[{}] onClose", key_);
  pool_ready_ = false;

  delete_callback_(key_);

  if (upstream_handle_) {
    ASSERT(!conn_data_);
    upstream_handle_->cancel(Tcp::ConnectionPool::CancelPolicy::Default);
    upstream_handle_ = nullptr;
    ENVOY_LOG(debug, "UpstreamHandlerImpl::onClose reset connection pool handler");
  }

  if (conn_data_) {
    conn_data_.reset();
    ENVOY_LOG(debug, "UpstreamHandlerImpl conn reset");
  }
}

int UpstreamHandlerImpl::start(Upstream::TcpPoolData& pool_data) {
  pool_ready_ = false;
  Tcp::ConnectionPool::Cancellable* handle = pool_data.newConnection(*this);
  if (handle) {
    ASSERT(upstream_handle_ == nullptr);
    upstream_handle_ = handle;
  }
  return 0;
}

void UpstreamHandlerImpl::onPoolFailure(ConnectionPool::PoolFailureReason reason,
                                        absl::string_view transport_failure_reason,
                                        Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(error, "UpstreamHandlerImpl onPoolFailure [{}]:{} {} {}", key_,
            host->address()->asString(), static_cast<int>(reason), transport_failure_reason);
  upstream_handle_ = nullptr;
  delete_callback_(key_);

  for (auto upstream_request_callbacks : upstream_request_callbacks_) {
    if (upstream_request_callbacks) {
      upstream_request_callbacks->onPoolFailure(reason, transport_failure_reason, host);
    }
  }
  upstream_request_callbacks_.clear();
}

void UpstreamHandlerImpl::onPoolReady(Tcp::ConnectionPool::ConnectionDataPtr&& conn_data,
                                      Upstream::HostDescriptionConstSharedPtr host) {
  ENVOY_LOG(debug, "UpstreamHandlerImpl onPoolReady [{}]:{}", key_, host->address()->asString());
  upstream_host_ = host;
  conn_data_ = std::move(conn_data);
  upstream_handle_ = nullptr;
  conn_data_->addUpstreamCallbacks(*this);
  pool_ready_ = true;

  ENVOY_CONN_LOG(debug, "UpstreamHandlerImpl[{}]: upstream_request_callbacks_ size:{}",
                 conn_data_->connection(), key_, upstream_request_callbacks_.size());
  for (auto upstream_request_callbacks : upstream_request_callbacks_) {
    if (upstream_request_callbacks) {
      upstream_request_callbacks->onPoolReady(host);
    }
  }
  upstream_request_callbacks_.clear();
}

void UpstreamHandlerImpl::onData(Buffer::Instance& data, bool end_stream) {
  ASSERT(conn_data_);
  ASSERT(!upstream_handle_);
  ENVOY_CONN_LOG(debug, "UpstreamHandlerImpl[{}] data length:{}, end_stream:{}",
                 conn_data_->connection(), key_, data.length(), end_stream);
  conn_data_->connection().write(data, end_stream);
}

void UpstreamHandlerImpl::addResponseCallback(uint64_t request_id, ResponseCallback callback) {
  auto it = response_callbacks_.find(request_id);
  if (it != response_callbacks_.end()) {
    // if exist
    ENVOY_LOG(error, "addResponseCallback failed, request_id:{} already exist", request_id);
    return;
  }
  response_callbacks_.insert({request_id, callback});
}

bool UpstreamHandlerImpl::isPoolReady() { return pool_ready_; }

void UpstreamHandlerImpl::addUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) {
  upstream_request_callbacks_.push_back(callbacks);
}

void UpstreamHandlerImpl::removeUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) {
  upstream_request_callbacks_.erase(std::remove(upstream_request_callbacks_.begin(),
                                                upstream_request_callbacks_.end(), callbacks),
                                    upstream_request_callbacks_.end());
}

void UpstreamHandlerImpl::onUpstreamData(Buffer::Instance& data, bool end_stream) {
  ENVOY_LOG(debug, "UpstreamHandlerImpl[{}]: upstream callback length {} , end:{}", key_,
            data.length(), end_stream);
  if (!upstream_response_) {
    upstream_response_ = std::make_unique<UpstreamResponse>(config_, *this);
    upstream_response_->startUpstreamResponse();
  }

  UpstreamResponseStatus status = upstream_response_->upstreamData(data);
  switch (status) {
  case UpstreamResponseStatus::Complete: {
    ENVOY_LOG(debug, "meta protocol upstream handler: response complete");
    upstream_response_.reset();
    return;
  }
  case UpstreamResponseStatus::Reset: {
    ENVOY_LOG(debug, "meta protocol upstream handler: upstream reset");
    upstream_response_.reset();
    return;
  }
  case UpstreamResponseStatus::MoreData: {
    ENVOY_LOG(debug, "meta protocol upstream handler: need more data");
    return;
  }
  case UpstreamResponseStatus::Retry: {
    ENVOY_LOG(debug, "meta protocol upstream handler: retry");
    return;
  }
  }
}

void UpstreamHandlerImpl::onEvent(Envoy::Network::ConnectionEvent event) {
  ENVOY_LOG(debug, "UpstreamHandlerImpl[{}]: connection Event {}", key_, static_cast<int>(event));

  switch (event) {
  case Network::ConnectionEvent::RemoteClose:
    onClose();
    break;
  case Network::ConnectionEvent::LocalClose:
    onClose();
    break;
  default:
    // Connected is consumed by the connection pool.
    // NOT_REACHED_GCOVR_EXCL_LINE;
    break;
  }
}

void UpstreamHandlerImpl::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr) {
  ASSERT(metadata->getMessageType() == MessageType::Response ||
         metadata->getMessageType() == MessageType::Error);
  downstream_connection_.write(metadata->originMessage(), false);

  // callback by request id
  uint64_t request_id = metadata->getRequestId();
  auto it = response_callbacks_.find(request_id);
  if (it != response_callbacks_.end() && it->second) {
    ENVOY_LOG(debug, "meta protocol UpstreamHandlerImpl: id {} do response callback", request_id);
    it->second(metadata);
    // clear after callback
    response_callbacks_.erase(it);
  } else {
    ENVOY_LOG(debug, "meta protocol UpstreamHandlerImpl: id {} not set response callback",
              request_id);
  }

  ENVOY_LOG(debug,
            "meta protocol UpstreamHandlerImpl: complete processing of upstream response messages, "
            "id is {}",
            request_id);
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
