#pragma once

#include <any>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/thrift/protocol.h"
#include "src/application_protocols/thrift/transport.h"
#include "src/application_protocols/thrift/thrift.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Thrift {

#define ALL_PROTOCOL_STATES(FUNCTION)                                                              \
  FUNCTION(StopIteration)                                                                          \
  FUNCTION(WaitForData)                                                                            \
  FUNCTION(PassthroughData)                                                                        \
  FUNCTION(MessageBegin)                                                                           \
  FUNCTION(MessageEnd)                                                                             \
  FUNCTION(StructBegin)                                                                            \
  FUNCTION(StructEnd)                                                                              \
  FUNCTION(FieldBegin)                                                                             \
  FUNCTION(FieldValue)                                                                             \
  FUNCTION(FieldEnd)                                                                               \
  FUNCTION(MapBegin)                                                                               \
  FUNCTION(MapKey)                                                                                 \
  FUNCTION(MapValue)                                                                               \
  FUNCTION(MapEnd)                                                                                 \
  FUNCTION(ListBegin)                                                                              \
  FUNCTION(ListValue)                                                                              \
  FUNCTION(ListEnd)                                                                                \
  FUNCTION(SetBegin)                                                                               \
  FUNCTION(SetValue)                                                                               \
  FUNCTION(SetEnd)                                                                                 \
  FUNCTION(Done)

/**
 * ProtocolState represents a set of states used in a state machine to decode Thrift requests
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

/**
 * DecoderStateMachine is the Thrift message state machine as described in
 * source/extensions/filters/network/thrift_proxy/docs.
 */
class DecoderStateMachine : public Logger::Loggable<Logger::Id::thrift> {
public:
  DecoderStateMachine(ThriftProxy::Protocol& proto, ThriftProxy::MessageMetadataSharedPtr& metadata)
      : proto_(proto), metadata_(metadata), state_(ProtocolState::MessageBegin) {}

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
   * Set the current state. Used for testing only.
   */
  void setCurrentState(ProtocolState state) { state_ = state; }

  /**
   * @return the original message
   */
  Buffer::Instance& originalMessage() { return origin_message_; }

private:
  /**
   * Frame encodes information about the return state for nested elements, container element types,
   * and the number of remaining container elements.
   */
  struct Frame {
    Frame(ProtocolState state) : return_state_(state), elem_type_{}, value_type_{}, remaining_(0) {}
    Frame(ProtocolState state, ThriftProxy::FieldType elem_type)
        : return_state_(state), elem_type_(elem_type), value_type_{}, remaining_{} {}
    Frame(ProtocolState state, ThriftProxy::FieldType elem_type, uint32_t remaining)
        : return_state_(state), elem_type_(elem_type), value_type_{}, remaining_(remaining) {}
    Frame(ProtocolState state, ThriftProxy::FieldType key_type, ThriftProxy::FieldType value_type,
          uint32_t remaining)
        : return_state_(state), elem_type_(key_type), value_type_(value_type),
          remaining_(remaining) {}

    // Structs, lists, maps, and sets may be recursively nested in any combination. This field
    // indicates which state to return to at the completion of each of those types.
    const ProtocolState return_state_;

    // Indicates the element type for lists and sets or the key type for a map.
    const ThriftProxy::FieldType elem_type_;

    // Indicates the value type for a map.
    const ThriftProxy::FieldType value_type_;

    // Indicates the number of elements (or key-value pairs) remaining in a list, map, or set.
    uint32_t remaining_;
  };

  // These functions map directly to the matching ProtocolState values. Each returns the next state
  // or ProtocolState::WaitForData if more data is required.
  ProtocolState passthroughData(Buffer::Instance& buffer);
  ProtocolState messageBegin(Buffer::Instance& buffer);
  ProtocolState messageEnd(Buffer::Instance& buffer);
  ProtocolState structBegin(Buffer::Instance& buffer);
  ProtocolState structEnd(Buffer::Instance& buffer);
  ProtocolState fieldBegin(Buffer::Instance& buffer);
  ProtocolState fieldValue(Buffer::Instance& buffer);
  ProtocolState fieldEnd(Buffer::Instance& buffer);
  ProtocolState listBegin(Buffer::Instance& buffer);
  ProtocolState listValue(Buffer::Instance& buffer);
  ProtocolState listEnd(Buffer::Instance& buffer);
  ProtocolState mapBegin(Buffer::Instance& buffer);
  ProtocolState mapKey(Buffer::Instance& buffer);
  ProtocolState mapValue(Buffer::Instance& buffer);
  ProtocolState mapEnd(Buffer::Instance& buffer);
  ProtocolState setBegin(Buffer::Instance& buffer);
  ProtocolState setValue(Buffer::Instance& buffer);
  ProtocolState setEnd(Buffer::Instance& buffer);

  // handleValue represents the generic Value state from the state machine documentation. It
  // returns either ProtocolState::WaitForData if more data is required or the next state. For
  // structs, lists, maps, or sets the return_state is pushed onto the stack and the next state is
  // based on elem_type. For primitive value types, return_state is returned as the next state
  // (unless WaitForData is returned).
  ProtocolState handleValue(Buffer::Instance& buffer, ThriftProxy::FieldType elem_type,
                            ProtocolState return_state);

  // handleState delegates to the appropriate method based on state_.
  ProtocolState handleState(Buffer::Instance& buffer);

  // Helper method to retrieve the current frame's return state and remove the frame from the
  // stack.
  ProtocolState popReturnState();

  ThriftProxy::Protocol& proto_;
  ThriftProxy::MessageMetadataSharedPtr metadata_;
  ProtocolState state_;
  std::vector<Frame> stack_;
  uint32_t body_bytes_{};
  bool passthrough_enabled_{false}; // TODO enable passthrough in the codec config
  Buffer::OwnedImpl origin_message_;
};

using DecoderStateMachinePtr = std::unique_ptr<DecoderStateMachine>;

/**
 * Codec for Thrift protocol.
 */
class ThriftCodec : public MetaProtocolProxy::Codec, public Logger::Loggable<Logger::Id::filter> {
public:
  ThriftCodec() {
    transport_ =
        ThriftProxy::NamedTransportConfigFactory::getFactory(ThriftProxy::TransportType::Auto)
            .createTransport();
    protocol_ = ThriftProxy::NamedProtocolConfigFactory::getFactory(ThriftProxy::ProtocolType::Auto)
                    .createProtocol();
  };
  ~ThriftCodec() override = default;

  MetaProtocolProxy::DecodeStatus decode(Buffer::Instance& buffer,
                                         MetaProtocolProxy::Metadata& metadata) override;
  void encode(const MetaProtocolProxy::Metadata& metadata,
              const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) override;
  void onError(const MetaProtocolProxy::Metadata& metadata, const MetaProtocolProxy::Error& error,
               Buffer::Instance& buffer) override;

private:
  void toMetadata(const ThriftProxy::MessageMetadata& msgMetadata, Metadata& metadata);

  void toMsgMetadata(const Metadata& metadata, ThriftProxy::MessageMetadata& msgMetadata);

  void complete();

  ThriftProxy::TransportPtr transport_;
  ThriftProxy::ProtocolPtr protocol_;
  ThriftProxy::MessageMetadataSharedPtr metadata_;
  DecoderStateMachinePtr state_machine_;
  bool frame_started_{false};
  bool frame_ended_{false};
};

} // namespace Thrift
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
