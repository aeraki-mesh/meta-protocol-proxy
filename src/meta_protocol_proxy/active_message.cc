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
                                             std::string applicationProtocol, CodecPtr&& codec)
    : parent_(parent), stats_(stats), response_connection_(connection),
      application_protocol_(applicationProtocol), codec_(std::move(codec)),
      decoder_(std::make_unique<ResponseDecoder>(*codec_, *this)), complete_(false),
      response_status_(UpstreamResponseStatus::MoreData) {}

UpstreamResponseStatus ActiveResponseDecoder::onData(Buffer::Instance& data) {
  ENVOY_LOG(debug, "meta protocol {} response: the received reply data length is {}",
            application_protocol_, data.length());

  bool underflow = false;
  decoder_->onData(data, underflow);
  ASSERT(complete_ || underflow);

  return response_status_;
}

void ActiveResponseDecoder::onStreamDecoded(MetadataSharedPtr metadata,
                                            MutationSharedPtr mutation) {
  ASSERT(metadata->getMessageType() == MessageType::Response ||
         metadata->getMessageType() == MessageType::Error);
  // ASSERT(metadata->hasResponseStatus());

  metadata_ = metadata;
  if (applyMessageEncodedFilters(metadata, mutation) != FilterStatus::Continue) {
    response_status_ = UpstreamResponseStatus::Complete;
    return;
  }

  if (response_connection_.state() != Network::Connection::State::Open) {
    throw DownstreamConnectionCloseException("Downstream has closed or closing");
  }

  response_connection_.write(metadata->getOriginMessage(), false);
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
  case FilterStatus::StopIteration:
    break;
  case FilterStatus::Retry:
    response_status_ = UpstreamResponseStatus::Retry;
    decoder_->reset();
    break;
  default:
    ASSERT(FilterStatus::Continue == status);
    break;
  }

  return status;
}

// class ActiveMessageFilterBase
uint64_t ActiveMessageFilterBase::requestId() const { return parent_.requestId(); }

uint64_t ActiveMessageFilterBase::streamId() const { return parent_.streamId(); }

const Network::Connection* ActiveMessageFilterBase::connection() const {
  return parent_.connection();
}

Route::RouteConstSharedPtr ActiveMessageFilterBase::route() { return parent_.route(); }

Event::Dispatcher& ActiveMessageFilterBase::dispatcher() { return parent_.dispatcher(); }

void ActiveMessageFilterBase::resetStream() { parent_.resetStream(); }

StreamInfo::StreamInfo& ActiveMessageFilterBase::streamInfo() { return parent_.streamInfo(); }

// class ActiveMessageDecoderFilter
ActiveMessageDecoderFilter::ActiveMessageDecoderFilter(ActiveMessage& parent,
                                                       DecoderFilterSharedPtr filter,
                                                       bool dual_filter)
    : ActiveMessageFilterBase(parent, dual_filter), handle_(filter) {}

void ActiveMessageDecoderFilter::continueDecoding() {
  ASSERT(parent_.metadata());
  auto state = ActiveMessage::FilterIterationStartState::AlwaysStartFromNext;
  if (0 != parent_.metadata()->getOriginMessage().length()) {
    state = ActiveMessage::FilterIterationStartState::CanStartFromCurrent;
    ENVOY_LOG(warn, "The original message data is not consumed, triggering the decoder filter from "
                    "the current location");
  }
  const FilterStatus status = parent_.applyDecoderFilters(this, state);
  if (status == FilterStatus::Continue) {
    ENVOY_LOG(debug, "meta protocol response: start upstream");
    // All filters have been executed for the current decoder state.
    if (parent_.pendingStreamDecoded()) {
      // If the filter stack was paused during messageEnd, handle end-of-request details.
      parent_.finalizeRequest();
    }
    parent_.continueDecoding();
  }
}

void ActiveMessageDecoderFilter::sendLocalReply(const DirectResponse& response, bool end_stream) {
  parent_.sendLocalReply(response, end_stream);
}

void ActiveMessageDecoderFilter::startUpstreamResponse() { parent_.startUpstreamResponse(); }

UpstreamResponseStatus ActiveMessageDecoderFilter::upstreamData(Buffer::Instance& buffer) {
  return parent_.upstreamData(buffer);
}

void ActiveMessageDecoderFilter::resetDownstreamConnection() {
  parent_.resetDownstreamConnection();
}

// class ActiveMessageEncoderFilter
ActiveMessageEncoderFilter::ActiveMessageEncoderFilter(ActiveMessage& parent,
                                                       EncoderFilterSharedPtr filter,
                                                       bool dual_filter)
    : ActiveMessageFilterBase(parent, dual_filter), handle_(filter) {}

void ActiveMessageEncoderFilter::continueEncoding() {
  ASSERT(parent_.metadata());
  auto state = ActiveMessage::FilterIterationStartState::AlwaysStartFromNext;
  if (0 != parent_.metadata()->getOriginMessage().length()) {
    state = ActiveMessage::FilterIterationStartState::CanStartFromCurrent;
    ENVOY_LOG(warn, "The original message data is not consumed, triggering the encoder filter from "
                    "the current location");
  }
  const FilterStatus status = parent_.applyEncoderFilters(this, state);
  if (FilterStatus::Continue == status) {
    ENVOY_LOG(debug, "All encoding filters have been executed");
  }
}

// class ActiveMessage
ActiveMessage::ActiveMessage(ConnectionManager& parent)
    : parent_(parent), request_timer_(std::make_unique<Stats::HistogramCompletableTimespanImpl>(
                           parent_.stats().request_time_ms_, parent.timeSystem())),
      stream_id_(parent.randomGenerator().random()),
      stream_info_(parent.timeSystem(), parent_.connection().addressProviderSharedPtr()),
      pending_stream_decoded_(false), local_response_sent_(false) {
  parent_.stats().request_active_.inc();
}

ActiveMessage::~ActiveMessage() {
  parent_.stats().request_active_.dec();
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

void ActiveMessage::onStreamDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) {
  parent_.stats().request_decoding_success_.inc();

  metadata_ = metadata;
  filter_action_ = [metadata, mutation](DecoderFilter* filter) -> FilterStatus {
    return filter->onMessageDecoded(metadata, mutation);
  };

  auto status = applyDecoderFilters(nullptr, FilterIterationStartState::CanStartFromCurrent);
  if (status == FilterStatus::StopIteration) {
    ENVOY_LOG(debug, "meta protocol {} request: stop calling decoder filter, id is {}",
              parent_.config().applicationProtocol(), metadata->getRequestId());
    pending_stream_decoded_ = true;
    return;
  }

  finalizeRequest();

  ENVOY_LOG(
      debug,
      "meta protocol {} request: complete processing of downstream request messages, id is {}",
      parent_.config().applicationProtocol(), metadata->getRequestId());
}

void ActiveMessage::finalizeRequest() {
  pending_stream_decoded_ = false;
  parent_.stats().request_.inc();
  bool is_one_way = false;
  switch (metadata_->getMessageType()) {
  case MessageType::Request:
    parent_.stats().request_twoway_.inc();
    break;
  case MessageType::Oneway:
    parent_.stats().request_oneway_.inc();
    is_one_way = true;
    break;
  default:
    break;
  }

  if (local_response_sent_ || is_one_way) {
    parent_.deferredMessage(*this);
  }
}

void ActiveMessage::createFilterChain() {
  parent_.config().filterFactory().createFilterChain(*this);
}

MetaProtocolProxy::Route::RouteConstSharedPtr ActiveMessage::route() {
  if (cached_route_) {
    return cached_route_.value();
  }

  if (metadata_ != nullptr) {
    MetaProtocolProxy::Route::RouteConstSharedPtr route =
        parent_.config().routerConfig().route(*metadata_, stream_id_);
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
      if (local_response_sent_) {
        break;
      }

      if (status != FilterStatus::Continue) {
        return status;
      }
    }
  }

  filter_action_ = nullptr;

  return FilterStatus::Continue;
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

      if (status != FilterStatus::Continue) {
        return status;
      }
    }
  }

  encoder_filter_action_ = nullptr;

  return FilterStatus::Continue;
}

void ActiveMessage::sendLocalReply(const DirectResponse& response, bool end_stream) {
  ASSERT(metadata_);
  // metadata_->setRequestId(request_id_);
  parent_.sendLocalReply(*metadata_, response, end_stream);

  if (end_stream) {
    return;
  }

  local_response_sent_ = true;
}

void ActiveMessage::startUpstreamResponse() {
  ENVOY_LOG(debug, "meta protocol response: start upstream");

  ASSERT(response_decoder_ == nullptr);

  auto codec = parent_.config().createCodec();

  // Create a response message decoder.
  response_decoder_ = std::make_unique<ActiveResponseDecoder>(
      *this, parent_.stats(), parent_.connection(), parent_.config().applicationProtocol(),
      std::move(codec));
}

UpstreamResponseStatus ActiveMessage::upstreamData(Buffer::Instance& buffer) {
  ASSERT(response_decoder_ != nullptr);

  try {
    auto status = response_decoder_->onData(buffer);
    if (status == UpstreamResponseStatus::Complete) {
      if (requestId() != response_decoder_->requestId()) {
        throw EnvoyException(fmt::format(
            "meta protocol {} response: request ID is not equal, {}:{}",
            parent_.config().applicationProtocol(), requestId(), response_decoder_->requestId()));
      }

      // Completed upstream response.
      parent_.deferredMessage(*this);
    } else if (status == UpstreamResponseStatus::Retry) {
      response_decoder_.reset();
    }

    return status;
  } catch (const DownstreamConnectionCloseException& ex) {
    ENVOY_CONN_LOG(error, "meta protocol {} response: exception ({})", parent_.connection(),
                   parent_.config().applicationProtocol(), ex.what());
    onReset();
    parent_.stats().response_error_caused_connection_close_.inc();
    return UpstreamResponseStatus::Reset;
  } catch (const EnvoyException& ex) {
    ENVOY_CONN_LOG(error, "meta protocol {} response: exception ({})", parent_.connection(),
                   parent_.config().applicationProtocol(), ex.what());
    parent_.stats().response_decoding_error_.inc();

    onError(ex.what());
    return UpstreamResponseStatus::Reset;
  }
}

void ActiveMessage::resetDownstreamConnection() {
  parent_.connection().close(Network::ConnectionCloseType::NoFlush);
}

void ActiveMessage::resetStream() { parent_.deferredMessage(*this); }

uint64_t ActiveMessage::requestId() const {
  return metadata_ != nullptr ? metadata_->getRequestId() : 0;
}

uint64_t ActiveMessage::streamId() const { return stream_id_; }

void ActiveMessage::continueDecoding() { parent_.continueDecoding(); }

StreamInfo::StreamInfo& ActiveMessage::streamInfo() { return stream_info_; }

Event::Dispatcher& ActiveMessage::dispatcher() { return parent_.connection().dispatcher(); }

const Network::Connection* ActiveMessage::connection() const { return &parent_.connection(); }

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

void ActiveMessage::onReset() { parent_.deferredMessage(*this); }

void ActiveMessage::onError(const std::string& what) {
  if (!metadata_) {
    // It's possible that an error occurred before the decoder generated metadata,
    // and a metadata object needs to be created in order to generate a local reply.
    metadata_ = std::make_shared<MetadataImpl>();
  }

  ASSERT(metadata_);
  ENVOY_LOG(error, "Bad response: {}", what);
  sendLocalReply(AppException(Error{ErrorType::BadResponse, what}), false);
  parent_.deferredMessage(*this);
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
