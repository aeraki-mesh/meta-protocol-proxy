#include <any>

#include "envoy/buffer/buffer.h"

#include "common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/trpc/trpc_codec.h"
#include "src/application_protocols/trpc/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

MetaProtocolProxy::DecodeStatus TrpcCodec::decode(Buffer::Instance& buffer,
                                                  MetaProtocolProxy::Metadata& metadata) {
  ENVOY_LOG(debug, "trpc decoder: {} bytes available", buffer.length());
  // https://git.code.oa.com/trpc/trpc-protocol/blob/master/docs/protocol_design.md
  messageType_ = metadata.getMessageType();
  ASSERT(messageType_ == MetaProtocolProxy::MessageType::Request ||
         messageType_ == MetaProtocolProxy::MessageType::Response);

  auto state = decoder_base_.onData(buffer);

  if (state == CodecChecker::DecodeStage::kWaitForData) {
    ENVOY_LOG(debug, "trpc decoder: wait for data");
    return DecodeStatus::WaitForData;
  }
  ASSERT(state == CodecChecker::DecodeStage::kDecodeDone);
  toMetadata(metadata);
  return DecodeStatus::Done;
}

void TrpcCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                       const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  // TODO we don't need to implement encode for now
  (void)metadata;
  (void)mutation;
  (void)buffer;
}

void TrpcCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  TrpcResponseProtocol response_protocol;
  // rsp_header_
  auto& header = response_protocol.protocol_header_;
  header.set_request_id(metadata.getRequestId());
  header.set_call_type(metadata.getUint32("call_type"));
  header.set_version(metadata.getUint32("version"));
  header.set_content_type(metadata.getUint32("content_type"));
  header.set_content_type(metadata.getUint32("content_encoding"));

  int32_t errCode;
  switch (error.type) {
  case MetaProtocolProxy::ErrorType::RouteNotFound:
    errCode = trpc::TRPC_SERVER_NOSERVICE_ERR;
    break;
  default:
    errCode = trpc::TRPC_SERVER_SYSTEM_ERR;
  }
  header.set_error_msg(error.message);
  header.set_ret(errCode);
  header.set_func_ret(errCode);
  // 不需要rsp_body

  response_protocol.encode(buffer);
}

void TrpcCodec::onFixedHeaderDecoded(std::unique_ptr<TrpcFixedHeader> fixed_header) {
  (void)fixed_header;
}

bool TrpcCodec::onDecodeRequestOrResponseProtocol(std::string&& header_raw) {
  if (messageType_ == MetaProtocolProxy::MessageType::Request) {
    if (!requestHeader_.ParseFromString(header_raw)) {
      return false;
    }
    return true;
  }
  if (!responseHeader_.ParseFromString(header_raw)) {
    return false;
  }
  return true;
}

void TrpcCodec::onCompleted(std::unique_ptr<Buffer::OwnedImpl> buffer) {
  origin_msg_ = std::move(buffer);
}

void TrpcCodec::toMetadata(MetaProtocolProxy::Metadata& metadata) {
  if (messageType_ == MetaProtocolProxy::MessageType::Request) {
    metadata.setRequestId(requestHeader_.request_id());
    metadata.putString("caller",requestHeader_.caller());
    metadata.putString("callee", requestHeader_.callee());
    metadata.putString("func", requestHeader_.func());

    metadata.put("call_type", requestHeader_.call_type());
    metadata.put("version", requestHeader_.version());
    metadata.put("content_type", requestHeader_.content_type());
    metadata.put("content_encoding", requestHeader_.content_encoding());

    for (auto const& kv : requestHeader_.trans_info()) {
      metadata.putString(kv.first, kv.second);
    }
  } else {
    metadata.setRequestId(responseHeader_.request_id());
    metadata.putString("error_msg", responseHeader_.error_msg());
    for (auto const& kv : responseHeader_.trans_info()) {
      metadata.putString(kv.first, kv.second);
    }
  }
  metadata.setOriginMessage(*origin_msg_);
}

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
