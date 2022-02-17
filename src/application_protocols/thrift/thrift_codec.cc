#include <any>

#include "envoy/buffer/buffer.h"

#include "common/common/logger.h"
#include "common/buffer/buffer_impl.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/thrift/thrift_codec.h"
#include "src/application_protocols/thrift/metadata.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Thrift {

MetaProtocolProxy::DecodeStatus ThriftCodec::decode(Buffer::Instance& data,
                                                    MetaProtocolProxy::Metadata& metadata) {

  ENVOY_LOG(debug, "thrift: {} bytes available", data.length());

  if (frame_ended_) {
    // Continuation after filter stopped iteration on transportComplete callback.
    complete();
    return MetaProtocolProxy::DecodeStatus::Done;
  }

  if (!frame_started_) {
    // Look for start of next frame.
    if (!metadata_) {
      metadata_ = std::make_shared<ThriftProxy::MessageMetadata>();
    }

    if (!transport_->decodeFrameStart(data, *metadata_)) {
      ENVOY_LOG(debug, "thrift: need more data for {} transport start", transport_->name());
      return MetaProtocolProxy::DecodeStatus::WaitForData;
    }
    ENVOY_LOG(debug, "thrift: {} transport started", transport_->name());

    if (metadata_->hasProtocol()) {
      if (protocol_->type() == ThriftProxy::ProtocolType::Auto) {
        protocol_->setType(metadata_->protocol());
        ENVOY_LOG(debug, "thrift: {} transport forced {} protocol", transport_->name(),
                  protocol_->name());
      } else if (metadata_->protocol() != protocol_->type()) {
        throw EnvoyException(
            fmt::format("transport reports protocol {}, but configured for {}",
                        ThriftProxy::ProtocolNames::get().fromType(metadata_->protocol()),
                        ThriftProxy::ProtocolNames::get().fromType(protocol_->type())));
      }
    }
    if (metadata_->hasAppException()) {
      ThriftProxy::AppExceptionType ex_type = metadata_->appExceptionType();
      std::string ex_msg = metadata_->appExceptionMessage();
      // Force new metadata if we get called again.
      metadata_.reset();
      throw EnvoyException(
          fmt::format("thrift AppException: type: {}, message: {}", ex_type, ex_msg));
    }

    frame_started_ = true;
    state_machine_ = std::make_unique<DecoderStateMachine>(*protocol_, *metadata_);
  }

  ASSERT(state_machine_ != nullptr);

  ENVOY_LOG(debug, "thrift: protocol {}, state {}, {} bytes available", protocol_->name(),
            ProtocolStateNameValues::name(state_machine_->currentState()), data.length());

  ProtocolState rv = state_machine_->run(data);
  if (rv == ProtocolState::WaitForData) {
    ENVOY_LOG(debug, "thrift: wait for data");
    return DecodeStatus::WaitForData;
  }

  ASSERT(rv == ProtocolState::Done);

  // Message complete, decode end of frame.
  if (!transport_->decodeFrameEnd(data)) {
    ENVOY_LOG(debug, "thrift: need more data for {} transport end", transport_->name());
    return DecodeStatus::WaitForData;
  }

  toMetadata(*metadata_, metadata);
  ENVOY_LOG(debug, "thrift: origin message length {}  ", metadata.getOriginMessage().length());

  frame_ended_ = true;
  metadata_.reset();

  ENVOY_LOG(debug, "thrift: {} transport ended", transport_->name());

  // Reset for next frame.
  complete();
  return DecodeStatus::Done;
}

void ThriftCodec::complete() {
  state_machine_ = nullptr;
  frame_started_ = false;
  frame_ended_ = false;
}

void ThriftCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                         const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  (void)mutation;
  (void)buffer;
  //ASSERT(buffer.length() == 0);
  switch (metadata.getMessageType()) {
  case MetaProtocolProxy::MessageType::Heartbeat: {
    break;
  }
  case MetaProtocolProxy::MessageType::Request: {
    // TODO
    break;
  }
  case MetaProtocolProxy::MessageType::Error: {
    // TODO
    break;
  }
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

static const std::string TApplicationException = "TApplicationException";
static const std::string MessageField = "message";
static const std::string TypeField = "type";
static const std::string StopField = "";

void ThriftCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                          const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  ASSERT(buffer.length() == 0);

  ThriftProxy::MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);

  // Handle cases where the exception occurs before the message name (e.g. some header transport
  // errors).
  if (!msgMetadata.hasMethodName()) {
    msgMetadata.setMethodName("");
  }
  if (!msgMetadata.hasSequenceId()) {
    msgMetadata.setSequenceId(0);
  }

  msgMetadata.setMessageType(ThriftProxy::MessageType::Exception);

  protocol_->writeMessageBegin(buffer, msgMetadata);
  protocol_->writeStructBegin(buffer, TApplicationException);

  protocol_->writeFieldBegin(buffer, MessageField, ThriftProxy::FieldType::String, 1);
  protocol_->writeString(buffer, error.message);
  protocol_->writeFieldEnd(buffer);

  protocol_->writeFieldBegin(buffer, TypeField, ThriftProxy::FieldType::I32, 2);
  protocol_->writeInt32(buffer, static_cast<int32_t>(ThriftProxy::AppExceptionType::InternalError));
  protocol_->writeFieldEnd(buffer);

  protocol_->writeFieldBegin(buffer, StopField, ThriftProxy::FieldType::Stop, 0);

  protocol_->writeStructEnd(buffer);
  protocol_->writeMessageEnd(buffer);
}

void ThriftCodec::toMetadata(const ThriftProxy::MessageMetadata& msgMetadata, Metadata& metadata) {
  if (msgMetadata.hasMethodName()) {
    metadata.putString("method", msgMetadata.methodName());
  }
  if (msgMetadata.hasSequenceId()) {
    metadata.setRequestId(msgMetadata.sequenceId());
  }

  ASSERT(msgMetadata.hasMessageType());
  switch (msgMetadata.messageType()) {
  case ThriftProxy::MessageType::Call: {
    metadata.setMessageType(MessageType::Request);
    break;
  }
  case ThriftProxy::MessageType::Reply: {
    metadata.setMessageType(MessageType::Response);
    break;
  }
  case ThriftProxy::MessageType::Oneway: {
    metadata.setMessageType(MessageType::Oneway);
    break;
  }
  case ThriftProxy::MessageType::Exception: {
    metadata.setMessageType(MessageType::Error);
    break;
  }
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  transport_->encodeFrame(metadata.getOriginMessage(), msgMetadata,
                          state_machine_->originalMessage());
}

void ThriftCodec::toMsgMetadata(const Metadata& metadata,
                                ThriftProxy::MessageMetadata& msgMetadata) {
  auto method = metadata.getString("method");
  // TODO we should use a more appropriate method to tell if metadata contains a specific key
  if (method != "") {
    msgMetadata.setMethodName(method);
  }
  msgMetadata.setSequenceId(metadata.getRequestId());
}

// PassthroughData -> PassthroughData
// PassthroughData -> MessageEnd (all body bytes received)
ProtocolState DecoderStateMachine::passthroughData(Buffer::Instance& buffer) {
  if (body_bytes_ > buffer.length()) {
    return ProtocolState::WaitForData;
  }

  origin_message_.move(buffer, body_bytes_);
  return ProtocolState::MessageEnd;
}

// MessageBegin -> StructBegin
ProtocolState DecoderStateMachine::messageBegin(Buffer::Instance& buffer) {
  const auto total = buffer.length();
  if (!proto_.readMessageBegin(buffer, metadata_)) {
    return ProtocolState::WaitForData;
  }

  stack_.clear();
  stack_.emplace_back(Frame(ProtocolState::MessageEnd));

  if (passthrough_enabled_) {
    body_bytes_ = metadata_.frameSize() - (total - buffer.length());
    return ProtocolState::PassthroughData;
  }

  proto_.writeMessageBegin(origin_message_, metadata_);
  return ProtocolState::StructBegin;
}

// MessageEnd -> Done
ProtocolState DecoderStateMachine::messageEnd(Buffer::Instance& buffer) {
  if (!proto_.readMessageEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeMessageEnd(origin_message_);
  return ProtocolState::Done;
}

// StructBegin -> FieldBegin
ProtocolState DecoderStateMachine::structBegin(Buffer::Instance& buffer) {
  std::string name;
  if (!proto_.readStructBegin(buffer, name)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeStructBegin(origin_message_, name);
  return ProtocolState::FieldBegin;
}

// StructEnd -> stack's return state
ProtocolState DecoderStateMachine::structEnd(Buffer::Instance& buffer) {
  if (!proto_.readStructEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeFieldBegin(origin_message_, "", ThriftProxy::FieldType::Stop, 0);
  proto_.writeStructEnd(origin_message_);
  return popReturnState();
}

// FieldBegin -> FieldValue, or
// FieldBegin -> StructEnd (stop field)
ProtocolState DecoderStateMachine::fieldBegin(Buffer::Instance& buffer) {
  std::string name;
  ThriftProxy::FieldType field_type;
  int16_t field_id;
  if (!proto_.readFieldBegin(buffer, name, field_type, field_id)) {
    return ProtocolState::WaitForData;
  }

  if (field_type == ThriftProxy::FieldType::Stop) {
    return ProtocolState::StructEnd;
  }

  stack_.emplace_back(Frame(ProtocolState::FieldEnd, field_type));

  proto_.writeFieldBegin(origin_message_, name, field_type, field_id);
  return ProtocolState::FieldValue;
}

// FieldValue -> FieldEnd (via stack return state)
ProtocolState DecoderStateMachine::fieldValue(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());

  Frame& frame = stack_.back();
  return handleValue(buffer, frame.elem_type_, frame.return_state_);
}

// FieldEnd -> FieldBegin
ProtocolState DecoderStateMachine::fieldEnd(Buffer::Instance& buffer) {
  if (!proto_.readFieldEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  popReturnState();

  proto_.writeFieldEnd(origin_message_);
  return ProtocolState::FieldBegin;
}

// ListBegin -> ListValue
ProtocolState DecoderStateMachine::listBegin(Buffer::Instance& buffer) {
  ThriftProxy::FieldType elem_type;
  uint32_t size;
  if (!proto_.readListBegin(buffer, elem_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::ListEnd, elem_type, size));

  proto_.writeListBegin(origin_message_, elem_type, size);
  return ProtocolState::ListValue;
}

// ListValue -> ListValue, ListBegin, MapBegin, SetBegin, StructBegin (depending on value type),
// or ListValue -> ListEnd
ProtocolState DecoderStateMachine::listValue(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());
  const uint32_t index = stack_.size() - 1;
  if (stack_[index].remaining_ == 0) {
    return popReturnState();
  }
  ProtocolState nextState = handleValue(buffer, stack_[index].elem_type_, ProtocolState::ListValue);
  if (nextState != ProtocolState::WaitForData) {
    stack_[index].remaining_--;
  }

  return nextState;
}

// ListEnd -> stack's return state
ProtocolState DecoderStateMachine::listEnd(Buffer::Instance& buffer) {
  if (!proto_.readListEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeListEnd(origin_message_);
  return popReturnState();
}

// MapBegin -> MapKey
ProtocolState DecoderStateMachine::mapBegin(Buffer::Instance& buffer) {
  ThriftProxy::FieldType key_type, value_type;
  uint32_t size;
  if (!proto_.readMapBegin(buffer, key_type, value_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::MapEnd, key_type, value_type, size));

  proto_.writeMapBegin(origin_message_, key_type, value_type, size);
  return ProtocolState::MapKey;
}

// MapKey -> MapValue, ListBegin, MapBegin, SetBegin, StructBegin (depending on key type), or
// MapKey -> MapEnd
ProtocolState DecoderStateMachine::mapKey(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());
  Frame& frame = stack_.back();
  if (frame.remaining_ == 0) {
    return popReturnState();
  }

  return handleValue(buffer, frame.elem_type_, ProtocolState::MapValue);
}

// MapValue -> MapKey, ListBegin, MapBegin, SetBegin, StructBegin (depending on value type), or
// MapValue -> MapKey
ProtocolState DecoderStateMachine::mapValue(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());
  const uint32_t index = stack_.size() - 1;
  ASSERT(stack_[index].remaining_ != 0);
  ProtocolState nextState = handleValue(buffer, stack_[index].value_type_, ProtocolState::MapKey);
  if (nextState != ProtocolState::WaitForData) {
    stack_[index].remaining_--;
  }

  return nextState;
}

// MapEnd -> stack's return state
ProtocolState DecoderStateMachine::mapEnd(Buffer::Instance& buffer) {
  if (!proto_.readMapEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeMapEnd(origin_message_);
  return popReturnState();
}

// SetBegin -> SetValue
ProtocolState DecoderStateMachine::setBegin(Buffer::Instance& buffer) {
  ThriftProxy::FieldType elem_type;
  uint32_t size;
  if (!proto_.readSetBegin(buffer, elem_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::SetEnd, elem_type, size));

  proto_.writeSetBegin(origin_message_, elem_type, size);
  return ProtocolState::SetValue;
}

// SetValue -> SetValue, ListBegin, MapBegin, SetBegin, StructBegin (depending on value type), or
// SetValue -> SetEnd
ProtocolState DecoderStateMachine::setValue(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());
  const uint32_t index = stack_.size() - 1;
  if (stack_[index].remaining_ == 0) {
    return popReturnState();
  }
  ProtocolState nextState = handleValue(buffer, stack_[index].elem_type_, ProtocolState::SetValue);
  if (nextState != ProtocolState::WaitForData) {
    stack_[index].remaining_--;
  }

  return nextState;
}

// SetEnd -> stack's return state
ProtocolState DecoderStateMachine::setEnd(Buffer::Instance& buffer) {
  if (!proto_.readSetEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  proto_.writeSetEnd(origin_message_);
  return popReturnState();
}

ProtocolState DecoderStateMachine::handleValue(Buffer::Instance& buffer,
                                               ThriftProxy::FieldType elem_type,
                                               ProtocolState return_state) {
  switch (elem_type) {
  case ThriftProxy::FieldType::Bool: {
    bool value{};
    if (proto_.readBool(buffer, value)) {
      proto_.writeBool(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::Byte: {
    uint8_t value{};
    if (proto_.readByte(buffer, value)) {
      proto_.writeByte(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::I16: {
    int16_t value{};
    if (proto_.readInt16(buffer, value)) {
      proto_.writeInt16(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::I32: {
    int32_t value{};
    if (proto_.readInt32(buffer, value)) {
      proto_.writeInt32(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::I64: {
    int64_t value{};
    if (proto_.readInt64(buffer, value)) {
      proto_.writeInt64(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::Double: {
    double value{};
    if (proto_.readDouble(buffer, value)) {
      proto_.writeDouble(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::String: {
    std::string value;
    if (proto_.readString(buffer, value)) {
      proto_.writeString(origin_message_, value);
      return return_state;
    }
    break;
  }
  case ThriftProxy::FieldType::Struct:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::StructBegin;
  case ThriftProxy::FieldType::Map:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::MapBegin;
  case ThriftProxy::FieldType::List:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::ListBegin;
  case ThriftProxy::FieldType::Set:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::SetBegin;
  default:
    throw EnvoyException(fmt::format("unknown field type {}", static_cast<int8_t>(elem_type)));
  }

  return ProtocolState::WaitForData;
}

ProtocolState DecoderStateMachine::handleState(Buffer::Instance& buffer) {
  switch (state_) {
  case ProtocolState::PassthroughData:
    return passthroughData(buffer);
  case ProtocolState::MessageBegin:
    return messageBegin(buffer);
  case ProtocolState::StructBegin:
    return structBegin(buffer);
  case ProtocolState::StructEnd:
    return structEnd(buffer);
  case ProtocolState::FieldBegin:
    return fieldBegin(buffer);
  case ProtocolState::FieldValue:
    return fieldValue(buffer);
  case ProtocolState::FieldEnd:
    return fieldEnd(buffer);
  case ProtocolState::ListBegin:
    return listBegin(buffer);
  case ProtocolState::ListValue:
    return listValue(buffer);
  case ProtocolState::ListEnd:
    return listEnd(buffer);
  case ProtocolState::MapBegin:
    return mapBegin(buffer);
  case ProtocolState::MapKey:
    return mapKey(buffer);
  case ProtocolState::MapValue:
    return mapValue(buffer);
  case ProtocolState::MapEnd:
    return mapEnd(buffer);
  case ProtocolState::SetBegin:
    return setBegin(buffer);
  case ProtocolState::SetValue:
    return setValue(buffer);
  case ProtocolState::SetEnd:
    return setEnd(buffer);
  case ProtocolState::MessageEnd:
    return messageEnd(buffer);
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

ProtocolState DecoderStateMachine::popReturnState() {
  ASSERT(!stack_.empty());
  ProtocolState return_state = stack_.back().return_state_;
  stack_.pop_back();
  return return_state;
}

ProtocolState DecoderStateMachine::run(Buffer::Instance& buffer) {
  while (state_ != ProtocolState::Done) {
    ENVOY_LOG(trace, "thrift: state {}, {} bytes available", ProtocolStateNameValues::name(state_),
              buffer.length());
    ENVOY_LOG(trace, "thrift: state {}, {} original message", ProtocolStateNameValues::name(state_),
              origin_message_.length());

    ProtocolState nextState = handleState(buffer);
    if (nextState == ProtocolState::WaitForData) {
      return ProtocolState::WaitForData;
    }

    state_ = nextState;
  }

  return state_;
}

} // namespace Thrift
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

