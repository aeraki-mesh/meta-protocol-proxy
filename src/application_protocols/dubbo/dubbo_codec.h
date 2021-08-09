#pragma once

#include <any>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

#include "source/common/common/logger.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/dubbo/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

#define ALL_PROTOCOL_STATES(FUNCTION)                                                              \
  FUNCTION(StopIteration)                                                                          \
  FUNCTION(WaitForData)                                                                            \
  FUNCTION(OnDecodeStreamHeader)                                                                   \
  FUNCTION(OnDecodeStreamData)                                                                     \
  FUNCTION(Done)

/**
 * ProtocolState represents a set of states used in a state machine to decode Dubbo requests
 * and responses.
 */
enum class ProtocolState { ALL_PROTOCOL_STATES(GENERATE_ENUM) };

class ProtocolStateNameValues {
public:
  static const std::string& name(ProtocolState state) {
    size_t i = static_cast<size_t>(state);
    ASSERT(i < names().size());
    return names()[i];
  }

private:
  static const std::vector<std::string>& names() {
    CONSTRUCT_ON_FIRST_USE(std::vector<std::string>, {ALL_PROTOCOL_STATES(GENERATE_STRING)});
  }
};

class DecoderStateMachine : public Logger::Loggable<Logger::Id::dubbo> {
public:

  DecoderStateMachine(Protocol& protocol)
      : protocol_(protocol), state_(ProtocolState::OnDecodeStreamHeader) {
    metadata_ = std::make_shared<MessageMetadata>();
  }

  /**
   * Consumes as much data from the configured Buffer as possible and executes the decoding state
   * machine. Returns ProtocolState::WaitForData if more data is required to complete processing of
   * a message. Returns ProtocolState::Done when the end of a message is successfully processed.
   * Once the Done state is reached, further invocations of run return immediately with Done.
   *
   * @param buffer a buffer containing the remaining data to be processed
   * @return ProtocolState returns with ProtocolState::WaitForData or ProtocolState::Done
   * @throw Envoy Exception if thrown by the underlying Protocol
   */
  ProtocolState run(Buffer::Instance& buffer);

  /**
   * @return the current ProtocolState
   */
  ProtocolState currentState() const { return state_; }

  /**
   * return the message metadata
   * @return
   */
  MessageMetadataSharedPtr messageMetadata() const {return metadata_;}
  /**
   * @return the message context
   */
  ContextSharedPtr messageContext() { return context_; }

private:
  // These functions map directly to the matching ProtocolState values. Each returns the next state
  // or ProtocolState::WaitForData if more data is required.
  ProtocolState onDecodeStreamHeader(Buffer::Instance& buffer);
  ProtocolState onDecodeStreamData(Buffer::Instance& buffer);

  // handleState delegates to the appropriate method based on state_.
  ProtocolState handleState(Buffer::Instance& buffer);

  Protocol& protocol_;
  MessageMetadataSharedPtr metadata_;
  ContextSharedPtr context_;
  ProtocolState state_;
};

using DecoderStateMachinePtr = std::unique_ptr<DecoderStateMachine>;

/**
 * Codec for Dubbo protocol.
 */
class DubboCodec : public MetaProtocolProxy::Codec, public Logger::Loggable<Logger::Id::dubbo> {
public:
  DubboCodec() {
    protocol_ = NamedProtocolConfigFactory::getFactory(ProtocolType::Dubbo)
                    .createProtocol(SerializationType::Hessian2);
  };
  ~DubboCodec() override = default;

  MetaProtocolProxy::DecodeStatus decode(Buffer::Instance& buffer,
                                         MetaProtocolProxy::Metadata& metadata) override;
  void encode(const MetaProtocolProxy::Metadata& metadata,
              const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) override;
  void onError(const MetaProtocolProxy::Metadata& metadata, const MetaProtocolProxy::Error& error,
               Buffer::Instance& buffer) override;

private:
  void toMetadata(const MessageMetadata& msgMetadata, MetaProtocolProxy::Metadata& metadata);

  void toMetadata(const MessageMetadata& msgMetadata, Context& context,
                  MetaProtocolProxy::Metadata& metadata);
  void toMsgMetadata(const MetaProtocolProxy::Metadata& metadata, MessageMetadata& msgMetadata);

  void start();

  void complete();

private:
  void encodeHeartbeat(const MetaProtocolProxy::Metadata& metadata, Buffer::Instance& buffer);

  ProtocolPtr protocol_;
  DecoderStateMachinePtr state_machine_;
  bool decode_started_{false};
};

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
