#include <any>

#include "envoy/buffer/buffer.h"

#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/dubbo/dubbo_codec.h"
#include "src/application_protocols/dubbo/protocol.h"
#include "src/application_protocols/dubbo/message.h"
#include "src/application_protocols/dubbo/message_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

MetaProtocolProxy::DecodeStatus DubboCodec::decode(Buffer::Instance& buffer,
                                                   MetaProtocolProxy::Metadata& metadata) {
  ENVOY_LOG(debug, "dubbo decoder: {} bytes available", buffer.length());

  if (!decode_started_) {
    start();
  }

  ENVOY_LOG(debug, "dubbo decoder: protocol {}, state {}, {} bytes available", protocol_->name(),
            ProtocolStateNameValues::name(state_machine_->currentState()), buffer.length());

  ProtocolState state = state_machine_->run(buffer);
  if (state == ProtocolState::WaitForData) {
    ENVOY_LOG(debug, "dubbo decoder: wait for data");
    return DecodeStatus::WaitForData;
  }

  ASSERT(state == ProtocolState::Done);

  toMetadata(*(state_machine_->messageMetadata()), *(state_machine_->messageContext()), metadata);

  // Reset for next request.
  complete();
  return DecodeStatus::Done;
}

void DubboCodec::start() {
  state_machine_ = std::make_unique<DecoderStateMachine>(*protocol_);
  decode_started_ = true;
}

void DubboCodec::complete() {
  state_machine_ = nullptr;
  decode_started_ = false;
}

void DubboCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "dubbo: codec server real address: {} ",
            metadata.getString(ReservedHeaders::RealServerAddress));

  switch (metadata.getMessageType()) {
  case MetaProtocolProxy::MessageType::Heartbeat: {
    encodeHeartbeat(metadata, buffer);
    break;
  }
  case MetaProtocolProxy::MessageType::Request: {
    encodeRequest(metadata, mutation, buffer);
    break;
  }
  case MetaProtocolProxy::MessageType::Response: {
    encodeResponse(metadata, mutation, buffer);
    break;
  }
  case MetaProtocolProxy::MessageType::Error: {
    break;
  }
  default:
    PANIC("not reached");
  }
}

void DubboCodec::encodeResponse(const MetaProtocolProxy::Metadata& metadata,
                                const MetaProtocolProxy::Mutation& mutation,
                                Buffer::Instance& buffer) {

  MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);
  ENVOY_LOG(
      debug,
      "dubbo: msgdata is hasRpcResultInfo {}, hasResponseStatus {}, headersize {}, bodysize {}",
      msgMetadata.hasRpcResultInfo(), msgMetadata.hasResponseStatus(), metadata.getHeaderSize(),
      metadata.getBodySize());

  bool has_mutation = false;
  if (msgMetadata.hasRpcResultInfo()) {
    auto* result = const_cast<RpcResultImpl*>(
        dynamic_cast<const RpcResultImpl*>(&msgMetadata.rpcResultInfo()));
    ENVOY_LOG(debug, "dubbo: codec result hasException {},result body {}", result->hasException(),
              result->getRspBody());
    if (result->attachment_ != nullptr) {
      ENVOY_LOG(debug, "dubbo: codec result attachment_ not null offset {}",
                result->attachment_->attachmentOffset(), result->getRspBody());
    }

    for (const auto& keyValue : mutation) {
      ENVOY_LOG(debug, "dubbo: encodeResponse codec mutation {} : {}", keyValue.first,
                keyValue.second);
      if (msgMetadata.hasRpcResultInfo()) {
        result->attachment_->remove(keyValue.first);
        result->attachment_->insert(keyValue.first, keyValue.second);
      }
      has_mutation = true;
    }

    ENVOY_LOG(debug, "dubbo: encodeResponse codec attachment is {}",
              result->attachment_->attachment().toDebugString());
  }

  if (has_mutation) {
    // upstream server has mutation header: x-envoy-peer-metadata-id x-envoy-peer-metadata
    // add the two headers add response
    ContextImpl ctx;
    ctx.setHeaderSize(metadata.getHeaderSize());
    ctx.setBodySize(metadata.getBodySize());
    if (!protocol_->encode(buffer, msgMetadata, ctx, "addheader")) {
      throw EnvoyException("failed to encode request message");
    }
  }
}

void DubboCodec::onError(const MetaProtocolProxy::Metadata& metadata,
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
    break;
  case MetaProtocolProxy::ErrorType::BadResponse:
    status = ResponseStatus::BadResponse;
    break;
  default:
    status = ResponseStatus::ServerError;
  }
  msgMetadata.setResponseStatus(status);
  ContextImpl ctx;
  if (!protocol_->encode(buffer, msgMetadata, ctx, error.message,
                         RpcResponseType::ResponseWithException)) {
    throw EnvoyException("failed to encode heartbeat message");
  }
}

void DubboCodec::toMetadata(const MessageMetadata& msgMetadata,
                            MetaProtocolProxy::Metadata& metadata) {
  if (msgMetadata.hasInvocationInfo()) {
    auto* invo = const_cast<RpcInvocationImpl*>(
        dynamic_cast<const RpcInvocationImpl*>(&msgMetadata.invocationInfo()));

    metadata.putString("interface", invo->serviceName());
    metadata.putString("method", invo->methodName());
    metadata.setOperationName(invo->serviceName() + "/" + invo->methodName());
    for (const auto& pair : invo->attachment().attachment()) {
      const auto key = pair.first->toString();
      const auto value = pair.second->toString();
      if (!key.has_value() || !value.has_value()) {
        continue;
      }
      metadata.putString(key.value(), value.value());
    }
  }
  metadata.put("InvocationInfo", msgMetadata.invocationInfoPtr());
  metadata.put("ProtocolType", msgMetadata.protocolType());
  metadata.put("ProtocolVersion", msgMetadata.protocolVersion());
  metadata.put("MessageType", msgMetadata.messageType());
  metadata.setRequestId(msgMetadata.requestId());
  auto timeout = msgMetadata.timeout();
  if (timeout.has_value()) {
    metadata.put("Timeout", msgMetadata.timeout());
  }
  metadata.put("TwoWay", msgMetadata.isTwoWay());
  metadata.put("SerializationType", msgMetadata.serializationType());
  if (msgMetadata.hasResponseStatus()) {
    metadata.put("ResponseStatus", msgMetadata.responseStatus());
  }
  if (msgMetadata.hasRpcResultInfo()) {
    auto* invo = const_cast<RpcResultImpl*>(
        dynamic_cast<const RpcResultImpl*>(&msgMetadata.rpcResultInfo()));
    for (const auto& pair : invo->attachment_->attachment()) {
      const auto key = pair.first->toString();
      const auto value = pair.second->toString();
      if (!key.has_value() || !value.has_value()) {
        continue;
      }
      metadata.putString(key.value(), value.value());
    }
    metadata.put("RpcResultInfo", msgMetadata.rpcResultInfoPtr());
  }

  switch (msgMetadata.messageType()) {
  case MessageType::Request:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Request);
    break;
  case MessageType::Response:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Response);
    break;
  case MessageType::Oneway:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Oneway);
    break;
  case MessageType::Exception:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Error);
    break;
  case MessageType::HeartbeatRequest:
    FALLTHRU;
  case MessageType::HeartbeatResponse:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Heartbeat);
    break;
  default:
    PANIC("not reached");
  }

  if (msgMetadata.hasResponseStatus()) {
    if (msgMetadata.responseStatus() == ResponseStatus::Ok) {
      metadata.setResponseStatus(MetaProtocolProxy::ResponseStatus::Ok);
    } else {
      metadata.setResponseStatus(MetaProtocolProxy::ResponseStatus::Error);
    }
  }
}
void DubboCodec::toMetadata(const MessageMetadata& msgMetadata, Context& context,
                            MetaProtocolProxy::Metadata& metadata) {
  DubboCodec::toMetadata(msgMetadata, metadata);
  metadata.setHeaderSize(context.headerSize());
  metadata.setBodySize(context.bodySize());
  metadata.originMessage().move(context.originMessage());
}

void DubboCodec::toMsgMetadata(const MetaProtocolProxy::Metadata& metadata,
                               MessageMetadata& msgMetadata) {
  msgMetadata.setRequestId(metadata.getRequestId());
  auto ref = metadata.getByKey("InvocationInfo");
  if (ref.has_value()) {
    const auto& invo = ref.value();
    msgMetadata.setInvocationInfo(std::any_cast<RpcInvocationSharedPtr>(invo));
  }

  ref = metadata.getByKey("RpcResultInfo");
  if (ref.has_value()) {
    const auto& result = ref.value();
    msgMetadata.setRpcResultInfo(std::any_cast<RpcResultSharedPtr>(result));
  }

  ref = metadata.getByKey("ProtocolType");
  assert(ref.has_value());
  const auto& proto_type = ref.value();
  msgMetadata.setProtocolType(std::any_cast<ProtocolType>(proto_type));

  ref = metadata.getByKey("ProtocolVersion");
  assert(ref.has_value());
  const auto& version = ref.value();
  msgMetadata.setProtocolVersion(std::any_cast<uint8_t>(version));

  ref = metadata.getByKey("MessageType");
  assert(ref.has_value());
  const auto& msg_type = ref.value();
  msgMetadata.setMessageType(std::any_cast<MessageType>(msg_type));

  ref = metadata.getByKey("Timeout");
  if (ref.has_value()) {
    const auto& timeout = ref.value();
    msgMetadata.setTimeout(std::any_cast<uint32_t>(timeout));
  }
  ref = metadata.getByKey("TwoWay");
  assert(ref.has_value());
  msgMetadata.setTwoWayFlag(metadata.getBool("TwoWay"));

  ref = metadata.getByKey("SerializationType");
  assert(ref.has_value());
  const auto& serial_type = ref.value();
  msgMetadata.setSerializationType(std::any_cast<SerializationType>(serial_type));
  ref = metadata.getByKey("ResponseStatus");
  if (ref.has_value()) {
    const auto& res_status = ref.value();
    msgMetadata.setResponseStatus(std::any_cast<ResponseStatus>(res_status));
  }
}

void DubboCodec::encodeHeartbeat(const MetaProtocolProxy::Metadata& metadata,
                                 Buffer::Instance& buffer) {
  MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);
  msgMetadata.setResponseStatus(ResponseStatus::Ok);
  msgMetadata.setMessageType(MessageType::HeartbeatResponse);
  ContextImpl ctx;
  if (!protocol_->encode(buffer, msgMetadata, ctx, "")) {
    throw EnvoyException("failed to encode heartbeat message");
  }
}

void DubboCodec::encodeRequest(const MetaProtocolProxy::Metadata& metadata,
                               const MetaProtocolProxy::Mutation& mutation,
                               Buffer::Instance& buffer) {
  MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);
  if (msgMetadata.hasInvocationInfo()) {
    auto* invo = const_cast<RpcInvocationImpl*>(
        dynamic_cast<const RpcInvocationImpl*>(&msgMetadata.invocationInfo()));
    for (const auto& keyValue : mutation) {
      ENVOY_LOG(debug, "dubbo: codec mutation {} : {}", keyValue.first, keyValue.second);
      invo->attachment().remove(keyValue.first);
      invo->attachment().insert(keyValue.first, keyValue.second);
    }

    ENVOY_LOG(debug, "dubbo: codec attachment is {}",
              invo->attachment().attachment().toDebugString());
  }
  ContextImpl ctx;
  ctx.setHeaderSize(metadata.getHeaderSize());
  ctx.setBodySize(metadata.getBodySize());
  if (!protocol_->encode(buffer, msgMetadata, ctx, "")) {
    throw EnvoyException("failed to encode request message");
  }
}

ProtocolState DecoderStateMachine::onDecodeStreamHeader(Buffer::Instance& buffer) {
  auto ret = protocol_.decodeHeader(buffer, metadata_);
  if (!ret.second) {
    ENVOY_LOG(debug, "dubbo decoder: need more data for {} protocol", protocol_.name());
    return ProtocolState::WaitForData;
  }

  context_ = ret.first;
  if (metadata_->messageType() == MessageType::HeartbeatRequest ||
      metadata_->messageType() == MessageType::HeartbeatResponse) {
    if (buffer.length() < (context_->headerSize() + context_->bodySize())) {
      ENVOY_LOG(debug, "dubbo decoder: need more data for {} protocol heartbeat", protocol_.name());
      return ProtocolState::WaitForData;
    }

    ENVOY_LOG(debug, "dubbo decoder: this is the {} heartbeat message", protocol_.name());
    context_->originMessage().move(buffer, (context_->headerSize() + context_->bodySize()));
    return ProtocolState::Done;
  }

  context_->originMessage().move(buffer, context_->headerSize());

  return ProtocolState::OnDecodeStreamData;
}

ProtocolState DecoderStateMachine::onDecodeStreamData(Buffer::Instance& buffer) {
  if (!protocol_.decodeData(buffer, context_, metadata_)) {
    ENVOY_LOG(debug, "dubbo decoder: need more data for {} serialization, current size {}",
              protocol_.serializer()->name(), buffer.length());
    return ProtocolState::WaitForData;
  }

  context_->originMessage().move(buffer, context_->bodySize());

  ENVOY_LOG(debug, "dubbo decoder: ends the deserialization of the message");
  return ProtocolState::Done;
}

ProtocolState DecoderStateMachine::handleState(Buffer::Instance& buffer) {
  switch (state_) {
  case ProtocolState::OnDecodeStreamHeader:
    return onDecodeStreamHeader(buffer);
  case ProtocolState::OnDecodeStreamData:
    return onDecodeStreamData(buffer);
  default:
    PANIC("not reached");
  }
}

ProtocolState DecoderStateMachine::run(Buffer::Instance& buffer) {
  while (state_ != ProtocolState::Done) {
    ENVOY_LOG(trace, "dubbo decoder: state {}, {} bytes available",
              ProtocolStateNameValues::name(state_), buffer.length());

    ProtocolState nextState = handleState(buffer);
    if (nextState == ProtocolState::WaitForData) {
      return ProtocolState::WaitForData;
    }

    state_ = nextState;
  }

  return state_;
}

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy