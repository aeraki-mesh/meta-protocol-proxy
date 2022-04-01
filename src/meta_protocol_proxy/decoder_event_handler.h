#pragma once

#include "envoy/common/pure.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

enum class FilterStatus : uint8_t {
  // Continue filter chain iteration.
  Continue,
  // Do not iterate to any of the remaining filters in the chain. Returning
  // FilterDataStatus::Continue from decodeData()/encodeData() or calling
  // continueDecoding()/continueEncoding() MUST be called if continued filter iteration is desired.
  StopIteration,
  // Indicates that a retry is required for the reply message received.
  Retry,
};

class MessageDecoder {
public:
  virtual ~MessageDecoder() = default;

  /**
   * Indicates that the message had been decoded.
   * @param metadata MessageMetadataSharedPtr describing the message
   * @return FilterStatus to indicate if filter chain iteration should continue
   */
  virtual FilterStatus onMessageDecoded(MetadataSharedPtr metadata,
                                        MutationSharedPtr mutation) PURE;
};

using MessageDecoderSharedPtr = std::shared_ptr<MessageDecoder>;

class MessageEncoder {
public:
  virtual ~MessageEncoder() = default;

  /**
   * Indicates that the message had been encoded.
   * @param metadata MessageMetadataSharedPtr describing the message
   * @param ctx the message context information
   * @return FilterStatus to indicate if filter chain iteration should continue
   */
  virtual FilterStatus onMessageEncoded(MetadataSharedPtr metadata,
                                        MutationSharedPtr mutation) PURE;
};

using MessageEncoderSharedPtr = std::shared_ptr<MessageEncoder>;

class MessageHandler {
public:
  virtual ~MessageHandler() = default;

  /**
   * Indicates that the message had been decoded.
   * @param metadata MessageMetadataSharedPtr describing the message
   * @param ctx the message context information
   * @return FilterStatus to indicate if filter chain iteration should continue
   */
  virtual void onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) PURE;
};

using MessageDecoderSharedPtr = std::shared_ptr<MessageDecoder>;

class DecoderCallbacksBase {
public:
  virtual ~DecoderCallbacksBase() = default;

  /**
   * @return newMessageHandler* a new MessageHandler for a message.
   */
  virtual MessageHandler& newMessageHandler() PURE;

  /**
   * Indicates that the message is a heartbeat.
   */
  virtual void onHeartbeat(MetadataSharedPtr) PURE;
};

class RequestDecoderCallbacks : public DecoderCallbacksBase {};
class ResponseDecoderCallbacks : public DecoderCallbacksBase {};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
