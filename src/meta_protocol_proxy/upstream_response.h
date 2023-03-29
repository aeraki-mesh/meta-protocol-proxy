#pragma once

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/decoder_event_handler.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/config_interface.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
class UpstreamResponse;
class UpstreamHandlerResponseDecoder : public ResponseDecoderCallbacks,
                                       public MessageHandler,
                                       Logger::Loggable<Logger::Id::filter> {
public:
  UpstreamHandlerResponseDecoder(MessageHandler& handler, CodecPtr codec)
      : handler_(handler), codec_(std::move(codec)),
        decoder_(std::make_unique<ResponseDecoder>(*codec_, *this)) {}

  UpstreamResponseStatus decode(Buffer::Instance& data) {
    ENVOY_LOG(debug, "meta protocol response: the received reply data length is {}", data.length());

    bool underflow = false;
    try {
      decoder_->onData(data, underflow);
    } catch (const EnvoyException& ex) {
      ENVOY_LOG(error, "meta protocol error: {}", ex.what());
      return UpstreamResponseStatus::Reset;
    }

    // decoder return underflow in th following two cases:
    // 1. decoder needs more data to complete the decoding of the current response, in this case,
    // the buffer contains part of the incomplete response.
    // 2. the response in the buffer have been processed and the buffer is already empty.
    //
    // Since underflow is also true when a response is completed, we need to use complete_ instead
    // of underflow to check whether the current response is completed or not.
    ASSERT(complete_ || underflow);
    return complete_ ? UpstreamResponseStatus::Complete : UpstreamResponseStatus::MoreData;
  }

  // MessageHandler
  void onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override;

  // ResponseDecoderCallbacks
  MessageHandler& newMessageHandler() override { return *this; };
  // Ignore the heartBeat from the upstream
  bool onHeartbeat(MetadataSharedPtr) override { return true; };

private:
  MessageHandler& handler_;
  CodecPtr codec_;
  ResponseDecoderPtr decoder_;
  bool complete_ : 1;
};
using UpstreamHandlerResponseDecoderPtr = std::unique_ptr<UpstreamHandlerResponseDecoder>;

class UpstreamResponse : Logger::Loggable<Logger::Id::filter> {
public:
  UpstreamResponse(Config& config, MessageHandler& handler) : config_(config), handler_(handler) {}
  ~UpstreamResponse() = default;

  void startUpstreamResponse();
  UpstreamResponseStatus upstreamData(Buffer::Instance& buffer);

private:
  Config& config_;
  MessageHandler& handler_;
  UpstreamHandlerResponseDecoderPtr response_decoder_;
};
using UpstreamResponsePtr = std::unique_ptr<UpstreamResponse>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
