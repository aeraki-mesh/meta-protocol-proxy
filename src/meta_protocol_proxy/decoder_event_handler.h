#pragma once

#include "envoy/common/pure.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

enum class FilterStatus : uint8_t {
  // Continue filter chain iteration.
  ContinueIteration,
  // Pause iterating to any of the remaining filters in the chain.
  // The current message remains in the connection manager to wait to be continued.
  // ContinueDecoding()/continueEncoding() MUST be called to continue filter iteration.
  PauseIteration,
  // Abort the current iterate and remove the current message from the connection manager
  AbortIteration,
  // Indicates that a retry is required for the reply message received.
  Retry, // Retry has not been supported yet
};

class MessageDecoder {
public:
  virtual ~MessageDecoder() = default;

  /**
   * Indicates that the message had been decoded.
   * @param metadata MessageMetadataSharedPtr describing the message
   * @return FilterStatus to indicate if filter chain iteration should continue,pause,abort, or
   * retry
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
   * @return whether to continue waiting for response
   */
  virtual bool onHeartbeat(MetadataSharedPtr) PURE;
};

class RequestDecoderCallbacks : public DecoderCallbacksBase {};
class ResponseDecoderCallbacks : public DecoderCallbacksBase {};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
