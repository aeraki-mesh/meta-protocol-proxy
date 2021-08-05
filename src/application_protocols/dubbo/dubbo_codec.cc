#include <any>

#include "envoy/buffer/buffer.h"

#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/dubbo/dubbo_codec.h"
#include "src/application_protocols/dubbo/protocol.h"
#include "src/application_protocols/dubbo/message.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

MetaProtocolProxy::DecodeStatus DubboCodec::decode(Buffer::Instance& buffer,
                                                   MetaProtocolProxy::Metadata& metadata) {
  auto msgMetadata = std::make_shared<MessageMetadata>();
  auto ret = protocol_->decodeHeader(buffer, msgMetadata);
  if (!ret.second) {
    ENVOY_LOG(debug, "dubbo decoder: need more data for {} protocol", protocol_->name());
    return MetaProtocolProxy::DecodeStatus::WaitForData;
  }

  auto context = ret.first;
  if (msgMetadata->messageType() == MessageType::HeartbeatRequest ||
      msgMetadata->messageType() == MessageType::HeartbeatResponse) {
    if (buffer.length() < (context->headerSize() + context->bodySize())) {
      ENVOY_LOG(debug, "dubbo decoder: need more data for {} protocol heartbeat",
                protocol_->name());
      return MetaProtocolProxy::DecodeStatus::WaitForData;
    }
    context->originMessage().move(buffer, (context->headerSize() + context->bodySize()));
  }

  context->originMessage().move(buffer, context->headerSize());
  if (!protocol_->decodeData(buffer, ret.first, msgMetadata)) {
    ENVOY_LOG(debug, "dubbo decoder: need more data for {} serialization, current size {}",
              protocol_->serializer()->name(), buffer.length());
    return MetaProtocolProxy::DecodeStatus::WaitForData;
  }
  context->originMessage().move(buffer, context->bodySize());
  this->toMetadata(*msgMetadata, *context, metadata);
  return MetaProtocolProxy::DecodeStatus::Done;
}

void DubboCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  (void)mutation;
  ASSERT(buffer.length() == 0);
  switch (metadata.getMessageType()) {
  case MetaProtocolProxy::MessageType::Heartbeat: {
    encodeHeartbeat(metadata, buffer);
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

void DubboCodec::toMetadata(const MessageMetadata& msgMetadata,
                            MetaProtocolProxy::Metadata& metadata) {
  if (msgMetadata.hasInvocationInfo()) {
    metadata.putString("interface", msgMetadata.invocationInfo().serviceName());
    metadata.putString("method", msgMetadata.invocationInfo().methodName());
    // TODO Add attachment to the metadata header
    metadata.put("InvocationInfo", msgMetadata.invocationInfoPtr());
  }
  metadata.put("ProtocolTyp", msgMetadata.protocolType());
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
  case MessageType::HeartbeatResponse:
    metadata.setMessageType(MetaProtocolProxy::MessageType::Heartbeat);
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
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
  metadata.setOriginMessage(context.originMessage());
}

void DubboCodec::toMsgMetadata(const MetaProtocolProxy::Metadata& metadata,
                               MessageMetadata& msgMetadata) {
  msgMetadata.setRequestId(metadata.getRequestId());
  auto ref = metadata.get("InvocationInfo");
  if (ref.has_value()) {
    msgMetadata.setInvocationInfo(std::any_cast<RpcInvocationSharedPtr>(ref.value()));
  }

  ref = metadata.get("ProtocolTyp");
  assert(ref.has_value());
  msgMetadata.setProtocolType(std::any_cast<ProtocolType>(ref.value()));

  ref = metadata.get("ProtocolVersion");
  assert(ref.has_value());
  msgMetadata.setProtocolVersion(std::any_cast<uint8_t>(ref.value()));

  ref = metadata.get("MessageType");
  assert(ref.has_value());
  msgMetadata.setMessageType(std::any_cast<MessageType>(ref.value()));

  ref = metadata.get("Timeout");
  if (ref.has_value()) {
    msgMetadata.setTimeout(std::any_cast<uint32_t>(ref.value()));
  }
  ref = metadata.get("TwoWay");
  assert(ref.has_value());
  msgMetadata.setTwoWayFlag(metadata.getBool("TwoWay"));

  ref = metadata.get("SerializationType");
  assert(ref.has_value());
  msgMetadata.setSerializationType(std::any_cast<SerializationType>(ref.value()));
  ref = metadata.get("ResponseStatus");
  if (ref.has_value()) {
    msgMetadata.setResponseStatus(std::any_cast<ResponseStatus>(ref.value()));
  }
}

void DubboCodec::encodeHeartbeat(const MetaProtocolProxy::Metadata& metadata,
                                 Buffer::Instance& buffer) {
  MessageMetadata msgMetadata;
  toMsgMetadata(metadata, msgMetadata);
  msgMetadata.setResponseStatus(ResponseStatus::Ok);
  msgMetadata.setMessageType(MessageType::HeartbeatResponse);
  if (!protocol_->encode(buffer, msgMetadata, "")) {
    throw EnvoyException("failed to encode heartbeat message");
  }
}

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
