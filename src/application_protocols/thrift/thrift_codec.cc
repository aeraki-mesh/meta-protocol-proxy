#include <any>

#include "envoy/buffer/buffer.h"

#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/thrift/thrift_codec.h"
#include "source/extensions/filters/network/thrift_proxy/metadata.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Thrift {

MetaProtocolProxy::DecodeStatus ThriftCodec::decode(Buffer::Instance& buffer,
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

    if (!transport_.decodeFrameStart(data, *metadata_)) {
      ENVOY_LOG(debug, "thrift: need more data for {} transport start", transport_.name());
      return MetaProtocolProxy::DecodeStatus::WaitForData;
    }
    ENVOY_LOG(debug, "thrift: {} transport started", transport_.name());

    if (metadata_->hasProtocol()) {
      if (protocol_.type() == ThriftProxy::ProtocolType::Auto) {
        protocol_.setType(metadata_->protocol());
        ENVOY_LOG(debug, "thrift: {} transport forced {} protocol", transport_.name(),
                  protocol_.name());
      } else if (metadata_->protocol() != protocol_.type()) {
        throw EnvoyException(
            fmt::format("transport reports protocol {}, but configured for {}",
                        ThriftProxy::ProtocolNames::get().fromType(metadata_->protocol()),
                        ThriftProxy::ProtocolNames::get().fromType(protocol_.type())));
      }
    }
    if (metadata_->hasAppException()) {
      ThriftProxy::AppExceptionType ex_type = metadata_->appExceptionType();
      std::string ex_msg = metadata_->appExceptionMessage();
      // Force new metadata if we get called again.
      metadata_.reset();
      throw EnvoyException(fmt::format("thrift AppException: type: {}, message: {}",ex_type,ex_msg);
    }

    frame_started_ = true;
    state_machine_ = std::make_unique<DecoderStateMachine>(protocol_, metadata_);
  }

  ASSERT(state_machine_ != nullptr);

  ENVOY_LOG(debug, "thrift: protocol {}, state {}, {} bytes available", protocol_.name(),
            ProtocolStateNameValues::name(state_machine_->currentState()), data.length());

  ProtocolState rv = state_machine_->run(data);
  if (rv == ProtocolState::WaitForData) {
    ENVOY_LOG(debug, "thrift: wait for data");
    return DecodeStatus::WaitForData;
  }

  ASSERT(rv == ProtocolState::Done);

  // Message complete, decode end of frame.
  if (!transport_.decodeFrameEnd(data)) {
    ENVOY_LOG(debug, "thrift: need more data for {} transport end", transport_.name());
    return DecodeStatus::WaitForData;
  }

  frame_ended_ = true;
  metadata_.reset();

  ENVOY_LOG(debug, "thrift: {} transport ended", transport_.name());

  // Reset for next frame.
  complete();
  toMetadata(metadata_, metadata);
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
  ASSERT(buffer.length() == 0);
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

void ThriftCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                          const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  ASSERT(buffer.length() == 0);
  MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);
  msgMetadata.setResponseStatus(ResponseStatus::Ok);
  msgMetadata.setMessageType(MessageType::Response);

  ResponseStatus status;
  switch (error.type) {
  case MetaProtocolProxy::ErrorType::RouteNotFound:
    status = ResponseStatus::ServiceNotFound;
  case MetaProtocolProxy::ErrorType::BadResponse:
    status = ResponseStatus::BadResponse;
  default:
    status = ResponseStatus::ServerError;
  }
  msgMetadata.setResponseStatus(status);
  if (!protocol_->encode(buffer, msgMetadata, error.message,
                         RpcResponseType::ResponseWithException)) {
    throw EnvoyException("failed to encode heartbeat message");
  }
}

void ThriftCodec::toMetadata(const ThriftProxy::MessageMetadata& msgMetadata, Metadata& metadata) {
  if (msgMetadata.hasMethodName()) {
    metadata.putString("method", msgMetadata.hasMethodName());
  }
}

void ThriftCodec::toMsgMetadata(const Metadata& metadata,
                                ThriftProxy::MessageMetadata& msgMetadata) {
  auto method = metadata.getString("method");
  if (method != "") { // TODO we should use a more appropriate method to tell if metadata contains a
                      // specific key
    msgMetadata.setMethodName();
  }
}

// PassthroughData -> PassthroughData
// PassthroughData -> MessageEnd (all body bytes received)
ProtocolState DecoderStateMachine::passthroughData(Buffer::Instance& buffer) {
  if (body_bytes_ > buffer.length()) {
    return ProtocolState::WaitForData;
  }

  Buffer::OwnedImpl body;
  body.move(buffer, body_bytes_);

  return ProtocolState::MessageEnd;
}

// MessageBegin -> StructBegin
ProtocolState DecoderStateMachine::messageBegin(Buffer::Instance& buffer) {
  const auto total = buffer.length();
  if (!proto_.readMessageBegin(buffer, *metadata_)) {
    return ProtocolState::WaitForData;
  }

  stack_.clear();
  stack_.emplace_back(Frame(ProtocolState::MessageEnd));

  const auto status = handler_.messageBegin(metadata_);

  if (passthrough_enabled_) {
    body_bytes_ = metadata_->frameSize() - (total - buffer.length());
    return {ProtocolState::PassthroughData, status};
  }

  return protocolState::StructBegin;
}

// MessageEnd -> Done
ProtocolState DecoderStateMachine::messageEnd(Buffer::Instance& buffer) {
  if (!proto_.readMessageEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  return ProtocolState::Done;
}

// StructBegin -> FieldBegin
ProtocolState DecoderStateMachine::structBegin(Buffer::Instance& buffer) {
  std::string name;
  if (!proto_.readStructBegin(buffer, name)) {
    return ProtocolState::WaitForData;
  }

  return ProtocolState::FieldBegin;
}

// StructEnd -> stack's return state
ProtocolState DecoderStateMachine::structEnd(Buffer::Instance& buffer) {
  if (!proto_.readStructEnd(buffer)) {
    return ProtocolState::WaitForData;
  }

  return popReturnState();
}

// FieldBegin -> FieldValue, or
// FieldBegin -> StructEnd (stop field)
ProtocolState DecoderStateMachine::fieldBegin(Buffer::Instance& buffer) {
  std::string name;
  FieldType field_type;
  int16_t field_id;
  if (!proto_.readFieldBegin(buffer, name, field_type, field_id)) {
    return ProtocolState::WaitForData;
  }

  if (field_type == FieldType::Stop) {
    return ProtocolState::StructEnd;
  }

  stack_.emplace_back(Frame(ProtocolState::FieldEnd, field_type));

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

  return ProtocolState::FieldBegin;
}

// ListBegin -> ListValue
ProtocolState DecoderStateMachine::listBegin(Buffer::Instance& buffer) {
  FieldType elem_type;
  uint32_t size;
  if (!proto_.readListBegin(buffer, elem_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::ListEnd, elem_type, size));

  return ProtocolState::ListValue;
}

// ListValue -> ListValue, ListBegin, MapBegin, SetBegin, StructBegin (depending on value type), or
// ListValue -> ListEnd
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

  return popReturnState();
}

// MapBegin -> MapKey
ProtocolState DecoderStateMachine::mapBegin(Buffer::Instance& buffer) {
  FieldType key_type, value_type;
  uint32_t size;
  if (!proto_.readMapBegin(buffer, key_type, value_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::MapEnd, key_type, value_type, size));

  return ProtocolState::MapKey;
}

// MapKey -> MapValue, ListBegin, MapBegin, SetBegin, StructBegin (depending on key type), or
// MapKey -> MapEnd
ProtocolState DecoderStateMachine::mapKey(Buffer::Instance& buffer) {
  ASSERT(!stack_.empty());
  Frame& frame = stack_.back();
  if (frame.remaining_ == 0) {
    return popReturnState(), FilterStatus::Continue;
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

  return popReturnState();
}

// SetBegin -> SetValue
ProtocolState DecoderStateMachine::setBegin(Buffer::Instance& buffer) {
  FieldType elem_type;
  uint32_t size;
  if (!proto_.readSetBegin(buffer, elem_type, size)) {
    return ProtocolState::WaitForData;
  }

  stack_.emplace_back(Frame(ProtocolState::SetEnd, elem_type, size));

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

  return popReturnState();
}

ProtocolState DecoderStateMachine::handleValue(Buffer::Instance& buffer, FieldType elem_type,
                                               ProtocolState return_state) {
  switch (elem_type) {
  case FieldType::Bool: {
    bool value{};
    if (proto_.readBool(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::Byte: {
    uint8_t value{};
    if (proto_.readByte(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::I16: {
    int16_t value{};
    if (proto_.readInt16(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::I32: {
    int32_t value{};
    if (proto_.readInt32(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::I64: {
    int64_t value{};
    if (proto_.readInt64(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::Double: {
    double value{};
    if (proto_.readDouble(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::String: {
    std::string value;
    if (proto_.readString(buffer, value)) {
      return return_state;
    }
    break;
  }
  case FieldType::Struct:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::StructBegin;
  case FieldType::Map:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::MapBegin;
  case FieldType::List:
    stack_.emplace_back(Frame(return_state));
    return ProtocolState::ListBegin;
  case FieldType::Set:
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
