#include "src/meta_protocol_proxy/active_message.h"
#include "src/meta_protocol_proxy/codec/codec.h"

#include "source/common/stats/timespan_impl.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

// class ActiveResponseDecoder
ActiveResponseDecoder::ActiveResponseDecoder(ActiveMessage& parent, MetaProtocolProxyStats& stats,
                                             Network::Connection& connection,
                                             std::string applicationProtocol, CodecPtr&& codec,
                                             Metadata& requestMetadata)
    : parent_(parent), stats_(stats), downstream_connection_(connection),
      application_protocol_(applicationProtocol), codec_(std::move(codec)),
      request_metadata_(requestMetadata),
      decoder_(std::make_unique<ResponseDecoder>(*codec_, *this)), complete_(false),
      response_status_(UpstreamResponseStatus::MoreData) {}

UpstreamResponseStatus ActiveResponseDecoder::onData(Buffer::Instance& data) {
  ENVOY_LOG(debug, "meta protocol {} response: the received reply data length is {}",
            application_protocol_, data.length());

  bool underflow = false;
  decoder_->onData(data, underflow);
  // decoder return underflow in th following two cases:
  // 1. decoder needs more data to complete the decoding of the current response, in this case,
  // the buffer contains part of the incomplete response.
  // 2. the response in the buffer have been processed and the buffer is already empty.
  //
  // Since underflow is also true when a response is completed, we need to use response_status_
  // instead of underflow to check whether the current response is completed or not.
  ASSERT(complete_ || underflow);

  return response_status_;
}

void ActiveResponseDecoder::onMessageDecoded(MetadataSharedPtr metadata,
                                             MutationSharedPtr mutation) {
  ASSERT(metadata->getMessageType() == MessageType::Response ||
         metadata->getMessageType() == MessageType::Error);

  metadata_ = metadata;
  if (applyMessageEncodedFilters(metadata, mutation) != FilterStatus::ContinueIteration) {
    response_status_ = UpstreamResponseStatus::Complete;
    return;
  }

  if (downstream_connection_.state() != Network::Connection::State::Open) {
    throw DownstreamConnectionCloseException("Downstream has closed or closing");
  }

  // put real server ip in the response
  metadata_->putString(Metadata::HEADER_REAL_SERVER_ADDRESS,
                       request_metadata_.getString(Metadata::HEADER_REAL_SERVER_ADDRESS));
  // TODO support response mutation
  codec_->encode(*metadata_, Mutation{}, metadata->originMessage());
  downstream_connection_.write(metadata->originMessage(), false);
  ENVOY_LOG(debug,
            "meta protocol {} response: the upstream response message has been forwarded to the "
            "downstream",
            application_protocol_);

  stats_.response_.inc();
  stats_.response_decoding_success_.inc();
  if (metadata->getMessageType() == MessageType::Error) {
    stats_.response_business_exception_.inc();
  }

  switch (metadata->getResponseStatus()) {
  case ResponseStatus::Ok:
    stats_.response_success_.inc();
    break;
  default:
    stats_.response_error_.inc();
    ENVOY_LOG(error, "meta protocol {} response status: {}", application_protocol_,
              metadata->getResponseStatus());
    break;
  }

  complete_ = true;
  response_status_ = UpstreamResponseStatus::Complete;

  ENVOY_LOG(
      debug,
      "meta protocol {} response: complete processing of upstream response messages, id is {}",
      application_protocol_, metadata->getRequestId());
}

FilterStatus ActiveResponseDecoder::applyMessageEncodedFilters(MetadataSharedPtr metadata,
                                                               MutationSharedPtr mutation) {
  parent_.encoder_filter_action_ = [metadata, mutation](EncoderFilter* filter) -> FilterStatus {
    return filter->onMessageEncoded(metadata, mutation);
  };

  auto status = parent_.applyEncoderFilters(
      nullptr, ActiveMessage::FilterIterationStartState::CanStartFromCurrent);
  switch (status) {
  case FilterStatus::PauseIteration:
    break;
  case FilterStatus::Retry:
    response_status_ = UpstreamResponseStatus::Retry;
    decoder_->reset();
    break;
  default:
    ASSERT(FilterStatus::ContinueIteration == status);
    break;
  }

  return status;
}

// class ActiveMessageFilterBase
uint64_t ActiveMessageFilterBase::requestId() const { return activeMessage_.requestId(); }

uint64_t ActiveMessageFilterBase::streamId() const { return activeMessage_.streamId(); }

const Network::Connection* ActiveMessageFilterBase::connection() const {
  return activeMessage_.connection();
}

Route::RouteConstSharedPtr ActiveMessageFilterBase::route() { return activeMessage_.route(); }

Event::Dispatcher& ActiveMessageFilterBase::dispatcher() { return activeMessage_.dispatcher(); }

void ActiveMessageFilterBase::resetStream() { activeMessage_.resetStream(); }

StreamInfo::StreamInfo& ActiveMessageFilterBase::streamInfo() {
  return activeMessage_.streamInfo();
}

// class ActiveMessageDecoderFilter
ActiveMessageDecoderFilter::ActiveMessageDecoderFilter(ActiveMessage& parent,
                                                       DecoderFilterSharedPtr filter,
                                                       bool dual_filter)
    : ActiveMessageFilterBase(parent, dual_filter), handle_(filter) {}

void ActiveMessageDecoderFilter::continueDecoding() {
  ASSERT(activeMessage_.metadata());
  ENVOY_LOG(debug, "meta protocol: continueDecoding, id is {}",
            activeMessage_.metadata()->getRequestId());
  auto state = ActiveMessage::FilterIterationStartState::AlwaysStartFromNext;
  if (0 != activeMessage_.metadata()->originMessage().length()) {
    state = ActiveMessage::FilterIterationStartState::CanStartFromCurrent;
    ENVOY_LOG(warn, "The original message data is not consumed, triggering the decoder filter from "
                    "the current location");
  }
  const FilterStatus status = activeMessage_.applyDecoderFilters(this, state);
  if (status == FilterStatus::ContinueIteration) {
    ENVOY_LOG(debug, "meta protocol response: start upstream");
    // All filters have been executed for the current decoder state.
    if (activeMessage_.pendingStreamDecoded()) {
      // If the filter stack was paused during messageEnd, handle end-of-request details.
      activeMessage_.maybeDeferredDeleteMessage();
    }
  }
}

void ActiveMessageDecoderFilter::sendLocalReply(const DirectResponse& response, bool end_stream) {
  activeMessage_.sendLocalReply(response, end_stream);
}

void ActiveMessageDecoderFilter::startUpstreamResponse(Metadata& requestMetadata) {
  activeMessage_.startUpstreamResponse(requestMetadata);
}

UpstreamResponseStatus ActiveMessageDecoderFilter::upstreamData(Buffer::Instance& buffer) {
  return activeMessage_.upstreamData(buffer);
}

void ActiveMessageDecoderFilter::resetDownstreamConnection() {
  activeMessage_.resetDownstreamConnection();
}

CodecPtr ActiveMessageDecoderFilter::createCodec() { return activeMessage_.createCodec(); }

void ActiveMessageDecoderFilter::setUpstreamConnection(
    Tcp::ConnectionPool::ConnectionDataPtr conn) {
  return activeMessage_.setUpstreamConnection(std::move(conn));
}

Tracing::MetaProtocolTracerSharedPtr ActiveMessageDecoderFilter::tracer() {
  return activeMessage_.tracer();
}

Tracing::TracingConfig* ActiveMessageDecoderFilter::tracingConfig() {
  return activeMessage_.tracingConfig();
}

// class ActiveMessageEncoderFilter
ActiveMessageEncoderFilter::ActiveMessageEncoderFilter(ActiveMessage& parent,
                                                       EncoderFilterSharedPtr filter,
                                                       bool dual_filter)
    : ActiveMessageFilterBase(parent, dual_filter), handle_(filter) {}

void ActiveMessageEncoderFilter::continueEncoding() {
  ASSERT(activeMessage_.metadata());
  auto state = ActiveMessage::FilterIterationStartState::AlwaysStartFromNext;
  if (0 != activeMessage_.metadata()->originMessage().length()) {
    state = ActiveMessage::FilterIterationStartState::CanStartFromCurrent;
    ENVOY_LOG(warn, "The original message data is not consumed, triggering the encoder filter from "
                    "the current location");
  }
  const FilterStatus status = activeMessage_.applyEncoderFilters(this, state);
  if (FilterStatus::ContinueIteration == status) {
    ENVOY_LOG(debug, "All encoding filters have been executed");
  }
}

// class ActiveMessage
ActiveMessage::ActiveMessage(ConnectionManager& connection_manager)
    : connection_manager_(connection_manager),
      request_timer_(std::make_unique<Stats::HistogramCompletableTimespanImpl>(
          connection_manager.stats().request_time_ms_, connection_manager.timeSystem())),
      stream_id_(
          connection_manager.randomGenerator().random()), // todo: we don't need stream id here?
      stream_info_(connection_manager.timeSystem(),
                   connection_manager.connection().connectionInfoProviderSharedPtr()),
      pending_stream_decoded_(false), local_response_sent_(false) {
  connection_manager.stats().request_active_.inc();
}

ActiveMessage::~ActiveMessage() {
  ENVOY_LOG(trace, "********** ActiveMessage destructed ***********");
  connection_manager_.stats().request_active_.dec();
  request_timer_->complete();
  for (auto& filter : decoder_filters_) {
    ENVOY_LOG(debug, "destroy decoder filter");
    filter->handler()->onDestroy();
  }

  for (auto& filter : encoder_filters_) {
    // Do not call on destroy twice for dual registered filters.
    if (!filter->dual_filter_) {
      ENVOY_LOG(debug, "destroy encoder filter");
      filter->handler()->onDestroy();
    }
  }
}

std::list<ActiveMessageEncoderFilterPtr>::iterator
ActiveMessage::commonEncodePrefix(ActiveMessageEncoderFilter* filter,
                                  FilterIterationStartState state) {
  // Only do base state setting on the initial call. Subsequent calls for filtering do not touch
  // the base state.
  if (filter == nullptr) {
    // ASSERT(!state_.local_complete_);
    // state_.local_complete_ = end_stream;
    return encoder_filters_.begin();
  }

  if (state == FilterIterationStartState::CanStartFromCurrent) {
    // The filter iteration has been stopped for all frame types, and now the iteration continues.
    // The current filter's encoding callback has not be called. Call it now.
    return filter->entry();
  }
  return std::next(filter->entry());
}

std::list<ActiveMessageDecoderFilterPtr>::iterator
ActiveMessage::commonDecodePrefix(ActiveMessageDecoderFilter* filter,
                                  FilterIterationStartState state) {
  if (!filter) {
    return decoder_filters_.begin();
  }
  if (state == FilterIterationStartState::CanStartFromCurrent) {
    // The filter iteration has been stopped for all frame types, and now the iteration continues.
    // The current filter's callback function has not been called. Call it now.
    return filter->entry();
  }
  return std::next(filter->entry());
}

void ActiveMessage::onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) {
  connection_manager_.stats().request_decoding_success_.inc();
  bool needApplyFilters = false;
  switch (metadata->getMessageType()) {
  case MessageType::Request:
    needApplyFilters = true;
    break;
  case MessageType::Stream_Init:
    needApplyFilters = true;
    connection_manager_.newActiveStream(metadata->getStreamId());
    break;
  case MessageType::Stream_Data:
    needApplyFilters = false;
    if (connection_manager_.streamExisted(metadata->getStreamId())) {
      ENVOY_LOG(debug,
                "meta protocol request: found an existing stream for stream data message, "
                "stream id: {}",
                metadata->getStreamId());
      Stream& existingStream = connection_manager_.getActiveStream(metadata->getStreamId());
      existingStream.send2upstream(metadata->originMessage());
    } else {
      ENVOY_LOG(error,
                "meta protocol request: can't find an existing stream for stream data message, "
                "stream id: {}",
                metadata->getStreamId());
    }
    break;
  case MessageType::Stream_Close_One_Way:
    needApplyFilters = false;
    if (connection_manager_.streamExisted(metadata->getStreamId())) {
      ENVOY_LOG(debug, "meta protocol: close client side stream {}", metadata->getStreamId());
      Stream& existingStream = connection_manager_.getActiveStream(metadata->getStreamId());
      // order matters, close stream before calling send2upstream
      existingStream.closeClientStream();
      existingStream.send2upstream(metadata->originMessage());
    } else {
      ENVOY_LOG(error,
                "meta protocol request: can't find an existing stream for stream close message, "
                "stream id: {}",
                metadata->getStreamId());
    }
    break;
  case MessageType::Stream_Close_Two_Way:
    needApplyFilters = false;
    if (connection_manager_.streamExisted(metadata->getStreamId())) {
      ENVOY_LOG(debug, "meta protocol: close the entire stream {}", metadata->getStreamId());
      Stream& existingStream = connection_manager_.getActiveStream(metadata->getStreamId());
      // order matters, close stream before calling send2upstream
      existingStream.closeClientStream();
      existingStream.closeServerStream();
      existingStream.send2upstream(metadata->originMessage());
    } else {
      ENVOY_LOG(error,
                "meta protocol request: can't find an existing stream for stream close message, "
                "stream id: {}",
                metadata->getStreamId());
    }
    break;
    // todo we need a timeout mechanism to remove a stream
  default:
    break;
  }

  metadata_ = metadata;
  // Apply filters for request/response RPC and the first message in a stream. Skip filters for all
  // the following messages in an existing stream
  if (needApplyFilters) {
    filter_action_ = [metadata, mutation](DecoderFilter* filter) -> FilterStatus {
      return filter->onMessageDecoded(metadata, mutation);
    };

    auto status = applyDecoderFilters(nullptr, FilterIterationStartState::CanStartFromCurrent);
    switch (status) {
    case FilterStatus::PauseIteration:
      ENVOY_LOG(debug, "meta protocol {} request: pause calling decoder filters, id is {}",
                connection_manager_.config().applicationProtocol(), metadata->getRequestId());
      pending_stream_decoded_ = true;
      break;
    case FilterStatus::AbortIteration:
      ENVOY_LOG(debug, "meta protocol {} request: abort calling decoder filters, id is {}",
                connection_manager_.config().applicationProtocol(), metadata->getRequestId());
      connection_manager_.deferredDeleteMessage(*this);
      break;
    case FilterStatus::ContinueIteration:
      maybeDeferredDeleteMessage();
      break;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }
  } else {
    maybeDeferredDeleteMessage();
  }

  ENVOY_LOG(
      debug,
      "meta protocol {} request: complete processing of downstream request messages, id is {}",
      connection_manager_.config().applicationProtocol(), metadata->getRequestId());
}

void ActiveMessage::setUpstreamConnection(Tcp::ConnectionPool::ConnectionDataPtr conn) {
  connection_manager_.getActiveStream(metadata_->getStreamId()).setUpstreamConn(std::move(conn));
}

Tracing::MetaProtocolTracerSharedPtr ActiveMessage::tracer() {
  return connection_manager_.tracer();
}

Tracing::TracingConfig* ActiveMessage::tracingConfig() {
  return connection_manager_.tracingConfig();
}

void ActiveMessage::maybeDeferredDeleteMessage() {
  pending_stream_decoded_ = false;
  connection_manager_.stats().request_.inc();
  bool is_one_way = false;
  switch (metadata_->getMessageType()) {
  case MessageType::Request:
    connection_manager_.stats().request_twoway_.inc();
    break;
  case MessageType::Oneway:
    connection_manager_.stats().request_oneway_.inc();
    is_one_way = true;
    break;
  case MessageType::Stream_Init:
    connection_manager_.stats().request_event_.inc();
    is_one_way = true;
    break;
    // stream responses are handled in the stream, so stream messages are one way
  case MessageType::Stream_Data:
  case MessageType::Stream_Close_One_Way:
  case MessageType::Stream_Close_Two_Way:
    is_one_way = true;
    break;
  default:
    break;
  }

  if (local_response_sent_ || is_one_way) {
    connection_manager_.deferredDeleteMessage(*this);
  }
}

void ActiveMessage::createFilterChain() {
  connection_manager_.config().filterFactory().createFilterChain(*this);
}

MetaProtocolProxy::Route::RouteConstSharedPtr ActiveMessage::route() {
  if (cached_route_) {
    return cached_route_.value();
  }

  if (metadata_ != nullptr) {
    MetaProtocolProxy::Route::RouteConstSharedPtr route =
        connection_manager_.config().routerConfig().route(*metadata_, stream_id_);
    cached_route_ = route;
    return cached_route_.value();
  }

  return nullptr;
}

FilterStatus ActiveMessage::applyDecoderFilters(ActiveMessageDecoderFilter* filter,
                                                FilterIterationStartState state) {
  ASSERT(filter_action_ != nullptr);
  if (!local_response_sent_) {
    for (auto entry = commonDecodePrefix(filter, state); entry != decoder_filters_.end(); entry++) {
      const FilterStatus status = filter_action_((*entry)->handler().get());
      // if a local response has been sent back to the upstream, the following filters in the chain
      // will not be executed. A local response is usually an error response in the framework layer,
      // sucha as no host found or rate limited request
      if (local_response_sent_) {
        break;
      }

      if (status != FilterStatus::ContinueIteration) {
        return status;
      }
    }
  }

  filter_action_ = nullptr;

  return FilterStatus::ContinueIteration;
}

FilterStatus ActiveMessage::applyEncoderFilters(ActiveMessageEncoderFilter* filter,
                                                FilterIterationStartState state) {
  ASSERT(encoder_filter_action_ != nullptr);

  if (!local_response_sent_) {
    for (auto entry = commonEncodePrefix(filter, state); entry != encoder_filters_.end(); entry++) {
      const FilterStatus status = encoder_filter_action_((*entry)->handler().get());
      if (local_response_sent_) {
        break;
      }

      if (status != FilterStatus::ContinueIteration) {
        return status;
      }
    }
  }

  encoder_filter_action_ = nullptr;

  return FilterStatus::ContinueIteration;
}

void ActiveMessage::sendLocalReply(const DirectResponse& response, bool end_stream) {
  ASSERT(metadata_);
  // metadata_->setRequestId(request_id_);
  connection_manager_.sendLocalReply(*metadata_, response, end_stream);

  if (end_stream) {
    return;
  }

  local_response_sent_ = true;
}

void ActiveMessage::startUpstreamResponse(Metadata& requestMetadata) {
  ENVOY_LOG(debug, "meta protocol response: start upstream");

  ASSERT(response_decoder_ == nullptr);

  CodecPtr codec = connection_manager_.config().createCodec();

  // Create a response message decoder.
  response_decoder_ = std::make_unique<ActiveResponseDecoder>(
      *this, connection_manager_.stats(), connection_manager_.connection(),
      connection_manager_.config().applicationProtocol(), std::move(codec), requestMetadata);
}

UpstreamResponseStatus ActiveMessage::upstreamData(Buffer::Instance& buffer) {
  ASSERT(response_decoder_ != nullptr);

  try {
    auto status = response_decoder_->onData(buffer);
    if (status == UpstreamResponseStatus::Complete) {
      if (requestId() != response_decoder_->requestId()) {
        throw EnvoyException(
            fmt::format("meta protocol {} response: request ID is not equal, {}:{}",
                        connection_manager_.config().applicationProtocol(), requestId(),
                        response_decoder_->requestId()));
      }

      // Completed upstream response.
      connection_manager_.deferredDeleteMessage(*this);
    } else if (status == UpstreamResponseStatus::Retry) {
      response_decoder_.reset();
    }

    return status;
  } catch (const DownstreamConnectionCloseException& ex) {
    ENVOY_CONN_LOG(error, "meta protocol {} response: exception ({})",
                   connection_manager_.connection(),
                   connection_manager_.config().applicationProtocol(), ex.what());
    onReset();
    connection_manager_.stats().response_error_caused_connection_close_.inc();
    return UpstreamResponseStatus::Reset;
  } catch (const EnvoyException& ex) {
    ENVOY_CONN_LOG(error, "meta protocol {} response: exception ({})",
                   connection_manager_.connection(),
                   connection_manager_.config().applicationProtocol(), ex.what());
    connection_manager_.stats().response_decoding_error_.inc();

    onError(ex.what());
    return UpstreamResponseStatus::Reset;
  }
}

void ActiveMessage::resetDownstreamConnection() {
  connection_manager_.connection().close(Network::ConnectionCloseType::NoFlush);
}

CodecPtr ActiveMessage::createCodec() { return connection_manager_.config().createCodec(); }

void ActiveMessage::resetStream() { connection_manager_.deferredDeleteMessage(*this); }

uint64_t ActiveMessage::requestId() const {
  return metadata_ != nullptr ? metadata_->getRequestId() : 0;
}

uint64_t ActiveMessage::streamId() const { return stream_id_; }

StreamInfo::StreamInfo& ActiveMessage::streamInfo() { return stream_info_; }

Event::Dispatcher& ActiveMessage::dispatcher() {
  return connection_manager_.connection().dispatcher();
}

const Network::Connection* ActiveMessage::connection() const {
  return &connection_manager_.connection();
}

void ActiveMessage::addDecoderFilter(DecoderFilterSharedPtr filter) {
  addDecoderFilterWorker(filter, false);
}

void ActiveMessage::addEncoderFilter(EncoderFilterSharedPtr filter) {
  addEncoderFilterWorker(filter, false);
}

void ActiveMessage::addFilter(CodecFilterSharedPtr filter) {
  addDecoderFilterWorker(filter, true);
  addEncoderFilterWorker(filter, true);
}

void ActiveMessage::addDecoderFilterWorker(DecoderFilterSharedPtr filter, bool dual_filter) {
  ActiveMessageDecoderFilterPtr wrapper =
      std::make_unique<ActiveMessageDecoderFilter>(*this, filter, dual_filter);
  filter->setDecoderFilterCallbacks(*wrapper);
  LinkedList::moveIntoListBack(std::move(wrapper), decoder_filters_);
}
void ActiveMessage::addEncoderFilterWorker(EncoderFilterSharedPtr filter, bool dual_filter) {
  ActiveMessageEncoderFilterPtr wrapper =
      std::make_unique<ActiveMessageEncoderFilter>(*this, filter, dual_filter);
  filter->setEncoderFilterCallbacks(*wrapper);
  LinkedList::moveIntoListBack(std::move(wrapper), encoder_filters_);
}

void ActiveMessage::onReset() { connection_manager_.deferredDeleteMessage(*this); }

void ActiveMessage::onError(const std::string& what) {
  if (!metadata_) {
    // It's possible that an error occurred before the decoder generated metadata,
    // and a metadata object needs to be created in order to generate a local reply.
    metadata_ = std::make_shared<MetadataImpl>();
  }

  ASSERT(metadata_);
  ENVOY_LOG(error, "Bad response: {}", what);
  sendLocalReply(AppException(Error{ErrorType::BadResponse, what}), false);
  connection_manager_.deferredDeleteMessage(*this);
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
