#include <any>

#include "envoy/buffer/buffer.h"

#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/brpc/brpc_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Brpc {

MetaProtocolProxy::DecodeStatus BrpcCodec::decode(Buffer::Instance& buffer,
                                                  MetaProtocolProxy::Metadata& metadata) {
  ENVOY_LOG(debug, "Brpc decoder: {} bytes available, msg type: {}", buffer.length(), metadata.getMessageType());
  messageType_ = metadata.getMessageType();
  ASSERT(messageType_ == MetaProtocolProxy::MessageType::Request ||
         messageType_ == MetaProtocolProxy::MessageType::Response);

  while (decode_status != BrpcDecodeStatus::DecodeDone) {
    decode_status = handleState(buffer);
    if (decode_status == BrpcDecodeStatus::WaitForData) {
      return DecodeStatus::WaitForData;
    }
  }

  // fill the metadata with the headers exacted from the message
  toMetadata(metadata);
  // reset decode status
  decode_status = BrpcDecodeStatus::DecodeHeader;
  return DecodeStatus::Done;
}

void BrpcCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                      const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  // TODO we don't need to implement encode for now.
  // This method only need to be implemented if we want to modify the respose message
  (void)metadata;
  (void)mutation;
  (void)buffer;
}

void BrpcCodec::onError(const MetaProtocolProxy::Metadata& /*metadata*/,
                        const MetaProtocolProxy::Error& /*error*/, Buffer::Instance& /*buffer*/) {
  // BrpcHeader response;
  // // Make sure to set the request id if the application protocol has one, otherwise MetaProtocol framework will 
  // // complaint that the id in the error response is not equal to the one in the request
  // response.set_pack_flow(metadata.getRequestId());
  // response.set_pack_len(BrpcHeader::HEADER_SIZE);
  // switch (error.type) {
  // case MetaProtocolProxy::ErrorType::RouteNotFound:
  //   response.set_rsp_code(static_cast<int16_t>(BrpcCode::NoRoute));
  //   break;
  // default:
  //   response.set_rsp_code(static_cast<int16_t>(BrpcCode::Error));
  //   break;
  // }
  // response.encode(buffer);
}

BrpcDecodeStatus BrpcCodec::handleState(Buffer::Instance& buffer) {
  switch (decode_status)
  {
  case BrpcDecodeStatus::DecodeHeader:
    return decodeHeader(buffer);
  case BrpcDecodeStatus::DecodePayload:
    return decodeBody(buffer);
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
  return BrpcDecodeStatus::DecodeDone;
}

BrpcDecodeStatus BrpcCodec::decodeHeader(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decode brpc header: {}", buffer.length());
  // Wait for more data if the header is not complete
  if (buffer.length() < BrpcHeader::HEADER_SIZE) {
    ENVOY_LOG(debug, "continue {}", buffer.length());
    return BrpcDecodeStatus::WaitForData;
  }

  if(!brpc_header_.decode(buffer)) {
    throw EnvoyException(fmt::format("brpc header invalid"));
  }
  
  return BrpcDecodeStatus::DecodePayload;
}

BrpcDecodeStatus BrpcCodec::decodeBody(Buffer::Instance& buffer) {
  // Wait for more data if the buffer is not a complete message	
  if (buffer.length() < BrpcHeader::HEADER_SIZE + brpc_header_.get_body_len()) {
    return BrpcDecodeStatus::WaitForData;
  }

  meta_.ParseFromArray(static_cast<uint8_t*>(buffer.linearize(BrpcHeader::HEADER_SIZE +
                                                              brpc_header_.get_meta_len())) +
                           BrpcHeader::HEADER_SIZE,
                       brpc_header_.get_meta_len());
  ENVOY_LOG(debug, "brpc meta: {}", meta_.DebugString());

  // move the decoded message out of the buffer
  origin_msg_ = std::make_unique<Buffer::OwnedImpl>();
  origin_msg_->move(buffer, BrpcHeader::HEADER_SIZE + brpc_header_.get_body_len());

  return BrpcDecodeStatus::DecodeDone;
}

void BrpcCodec::toMetadata(MetaProtocolProxy::Metadata& metadata) {
  // metadata.setRequestId(brpc_header_.get_pack_flow());
  // metadata.putString("cmd", std::to_string(brpc_header_.get_req_cmd()));
  metadata.setOriginMessage(*origin_msg_);
}

} // namespace Brpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
