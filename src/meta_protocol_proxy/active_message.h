#pragma once

#include "envoy/event/deferred_deletable.h"
#include "envoy/network/connection.h"
#include "envoy/network/filter.h"
#include "envoy/stats/timespan.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/linked_object.h"
#include "source/common/common/logger.h"
#include "source/common/stream_info/stream_info_impl.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/decoder_event_handler.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/route/route.h"
#include "src/meta_protocol_proxy/stats.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

class ConnectionManager;
class ActiveMessage;

class ActiveResponseDecoder : public ResponseDecoderCallbacks,
                              public StreamHandler,
                              Logger::Loggable<Logger::Id::filter> {
public:
  ActiveResponseDecoder(ActiveMessage& parent, MetaProtocolProxyStats& stats,
                        Network::Connection& connection, std::string applicationProtocol,
                        CodecPtr&& codec);
  ~ActiveResponseDecoder() override = default;

  UpstreamResponseStatus onData(Buffer::Instance& data);

  // StreamHandler
  void onStreamDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  // ResponseDecoderCallbacks
  StreamHandler& newStream() override { return *this; }
  void onHeartbeat(MetadataSharedPtr) override { NOT_IMPLEMENTED_GCOVR_EXCL_LINE; }

  uint64_t requestId() const { return metadata_ ? metadata_->getRequestId() : 0; }

private:
  FilterStatus applyMessageEncodedFilters(MetadataSharedPtr metadata, MutationSharedPtr mutation);

  ActiveMessage& parent_;
  MetaProtocolProxyStats& stats_;
  Network::Connection& response_connection_;
  std::string application_protocol_;
  CodecPtr codec_;
  ResponseDecoderPtr decoder_;
  MetadataSharedPtr metadata_;
  bool complete_ : 1;
  UpstreamResponseStatus response_status_;
};

using ActiveResponseDecoderPtr = std::unique_ptr<ActiveResponseDecoder>;

class ActiveMessageFilterBase : public virtual FilterCallbacksBase {
public:
  ActiveMessageFilterBase(ActiveMessage& parent, bool dual_filter)
      : parent_(parent), dual_filter_(dual_filter) {}
  ~ActiveMessageFilterBase() override = default;

  // FilterCallbacksBase
  uint64_t requestId() const override;
  uint64_t streamId() const override;
  const Network::Connection* connection() const override;
  MetaProtocolProxy::Route::RouteConstSharedPtr route() override;
  // SerializationType serializationType() const override;
  // ProtocolType protocolType() const override;
  StreamInfo::StreamInfo& streamInfo() override;
  Event::Dispatcher& dispatcher() override;
  void resetStream() override;

protected:
  ActiveMessage& parent_;
  const bool dual_filter_ : 1;
};

// Wraps a DecoderFilter and acts as the DecoderFilterCallbacks for the filter, enabling filter
// chain continuation.
class ActiveMessageDecoderFilter : public DecoderFilterCallbacks,
                                   public ActiveMessageFilterBase,
                                   public LinkedObject<ActiveMessageDecoderFilter>,
                                   Logger::Loggable<Logger::Id::filter> {
public:
  ActiveMessageDecoderFilter(ActiveMessage& parent, DecoderFilterSharedPtr filter,
                             bool dual_filter);
  ~ActiveMessageDecoderFilter() override = default;

  void continueDecoding() override;
  void sendLocalReply(const DirectResponse& response, bool end_stream) override;
  void startUpstreamResponse() override;
  UpstreamResponseStatus upstreamData(Buffer::Instance& buffer) override;
  void resetDownstreamConnection() override;

  DecoderFilterSharedPtr handler() { return handle_; }

private:
  DecoderFilterSharedPtr handle_;
};

using ActiveMessageDecoderFilterPtr = std::unique_ptr<ActiveMessageDecoderFilter>;

// Wraps a EncoderFilter and acts as the EncoderFilterCallbacks for the filter, enabling filter
// chain continuation.
class ActiveMessageEncoderFilter : public ActiveMessageFilterBase,
                                   public EncoderFilterCallbacks,
                                   public LinkedObject<ActiveMessageEncoderFilter>,
                                   Logger::Loggable<Logger::Id::filter> {
public:
  ActiveMessageEncoderFilter(ActiveMessage& parent, EncoderFilterSharedPtr filter,
                             bool dual_filter);
  ~ActiveMessageEncoderFilter() override = default;

  void continueEncoding() override;
  EncoderFilterSharedPtr handler() { return handle_; }

private:
  EncoderFilterSharedPtr handle_;

  friend class ActiveMessage;
};

using ActiveMessageEncoderFilterPtr = std::unique_ptr<ActiveMessageEncoderFilter>;

// ActiveMessage tracks downstream requests for which no response has been received.
class ActiveMessage : public LinkedObject<ActiveMessage>,
                      public Event::DeferredDeletable,
                      public StreamHandler,
                      public DecoderFilterCallbacks,
                      public FilterChainFactoryCallbacks,
                      Logger::Loggable<Logger::Id::filter> {
public:
  ActiveMessage(ConnectionManager& parent);
  ~ActiveMessage() override;

  // Indicates which filter to start the iteration with.
  enum class FilterIterationStartState { AlwaysStartFromNext, CanStartFromCurrent };

  // Returns the encoder filter to start iteration with.
  std::list<ActiveMessageEncoderFilterPtr>::iterator
  commonEncodePrefix(ActiveMessageEncoderFilter* filter, FilterIterationStartState state);
  // Returns the decoder filter to start iteration with.
  std::list<ActiveMessageDecoderFilterPtr>::iterator
  commonDecodePrefix(ActiveMessageDecoderFilter* filter, FilterIterationStartState state);

  // FilterChainFactoryCallbacks
  void addDecoderFilter(DecoderFilterSharedPtr filter) override;
  void addEncoderFilter(EncoderFilterSharedPtr filter) override;
  void addFilter(CodecFilterSharedPtr filter) override;

  // StreamHandler
  void onStreamDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  // DecoderFilterCallbacks
  uint64_t requestId() const override;
  uint64_t streamId() const override;
  const Network::Connection* connection() const override;
  void continueDecoding() override;
  // SerializationType serializationType() const override;
  // ProtocolType protocolType() const override;
  StreamInfo::StreamInfo& streamInfo() override;
  Route::RouteConstSharedPtr route() override;
  void sendLocalReply(const DirectResponse& response, bool end_stream) override;
  void startUpstreamResponse() override;
  UpstreamResponseStatus upstreamData(Buffer::Instance& buffer) override;
  void resetDownstreamConnection() override;
  Event::Dispatcher& dispatcher() override;
  void resetStream() override;

  void createFilterChain();
  FilterStatus applyDecoderFilters(ActiveMessageDecoderFilter* filter,
                                   FilterIterationStartState state);
  FilterStatus applyEncoderFilters(ActiveMessageEncoderFilter* filter,
                                   FilterIterationStartState state);
  void finalizeRequest();
  void onReset();
  void onError(const std::string& what);
  MetadataSharedPtr metadata() const { return metadata_; }
  // ContextSharedPtr context() const { return context_; }
  bool pendingStreamDecoded() const { return pending_stream_decoded_; }

private:
  void addDecoderFilterWorker(DecoderFilterSharedPtr filter, bool dual_filter);
  void addEncoderFilterWorker(EncoderFilterSharedPtr, bool dual_filter);

  ConnectionManager& parent_;

  MetadataSharedPtr metadata_;
  Stats::TimespanPtr request_timer_;
  ActiveResponseDecoderPtr response_decoder_;

  absl::optional<Route::RouteConstSharedPtr> cached_route_;

  std::list<ActiveMessageDecoderFilterPtr> decoder_filters_;
  std::function<FilterStatus(DecoderFilter*)> filter_action_;

  std::list<ActiveMessageEncoderFilterPtr> encoder_filters_;
  std::function<FilterStatus(EncoderFilter*)> encoder_filter_action_;

  // This value is used in the calculation of the weighted cluster.
  uint64_t stream_id_;
  StreamInfo::StreamInfoImpl stream_info_;

  Buffer::OwnedImpl response_buffer_;

  bool pending_stream_decoded_ : 1;
  bool local_response_sent_ : 1;

  friend class ActiveResponseDecoder;
};

using ActiveMessagePtr = std::unique_ptr<ActiveMessage>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
