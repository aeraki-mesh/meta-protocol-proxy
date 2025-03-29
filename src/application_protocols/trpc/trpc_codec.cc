#include <any>

#include "envoy/buffer/buffer.h"

#include "source/common/common/logger.h"

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
  if (mutation.size() < 1) {
    return;
  }
  for (const auto& keyValue : mutation) {
    ENVOY_LOG(debug, "trpc: codec mutation {} : {}", keyValue.first, keyValue.second);
  }
  if (metadata.getMessageType() == MetaProtocolProxy::MessageType::Request){
    TrpcRequestProtocol request_protocol;
    if (!request_protocol.mutateHeader(buffer, mutation)) {
      throw EnvoyException("encode request failed");
    }
  }else if(metadata.getMessageType() == MetaProtocolProxy::MessageType::Response) {
    TrpcResponseProtocol response_protocol;
    if (!response_protocol.mutateHeader(buffer, mutation)) {
      throw EnvoyException("encode response failed");
    }
  }
}

void TrpcCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  int32_t errCode;
  switch (error.type) {
  case MetaProtocolProxy::ErrorType::RouteNotFound:
    errCode = trpc::TRPC_SERVER_NOSERVICE_ERR;
    break;
  default:
    errCode = trpc::TRPC_SERVER_SYSTEM_ERR;
  }

  if (metadata.getMessageType() == MetaProtocolProxy::MessageType::Stream_Init) {
    TrpcStreamInitMeta stream_init;
    stream_init.fixed_header_.stream_id = metadata.getStreamId();
    stream_init.fixed_header_.data_frame_type = trpc::TrpcDataFrameType::TRPC_STREAM_FRAME;
    stream_init.fixed_header_.stream_frame_type = trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT;

    auto response_meta = stream_init.protocol_header_.mutable_response_meta();
    response_meta->set_ret(errCode);
    response_meta->set_error_msg(error.message);

    stream_init.encode(buffer);
  } else {
    TrpcResponseProtocol response_protocol;
    response_protocol.fixed_header_.data_frame_type = trpc::TrpcStreamFrameType::TRPC_UNARY;

    // rsp_header_
    auto& header = response_protocol.protocol_header_;
    header.set_request_id(metadata.getRequestId());
    //header.set_call_type(metadata.getUint32("call_type"));
    //header.set_version(metadata.getUint32("version"));
    //header.set_content_type(metadata.getUint32("content_type"));
    //header.set_content_type(metadata.getUint32("content_encoding"));
    header.set_error_msg(error.message);
    header.set_ret(errCode);
    header.set_func_ret(errCode);

    response_protocol.encode(buffer);
  }
}

void TrpcCodec::onFixedHeaderDecoded(std::unique_ptr<TrpcFixedHeader> fixed_header) {
  fixed_header_ = std::move(fixed_header);
  ENVOY_LOG(debug, "trpc decoder: stream id {}", fixed_header_->stream_id);
}

bool TrpcCodec::onUnaryHeader(std::string&& header_raw) {
  ASSERT(fixed_header_->stream_frame_type == trpc::TrpcStreamFrameType::TRPC_UNARY);
  if (messageType_ == MetaProtocolProxy::MessageType::Request) {
    return requestHeader_.ParseFromString(header_raw);
  }
  return responseHeader_.ParseFromString(header_raw);
}

bool TrpcCodec::onStreamFrame(std::string&& header_raw) {
  ASSERT(fixed_header_->stream_frame_type == trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT ||
         fixed_header_->stream_frame_type == trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE ||
         fixed_header_->stream_frame_type ==
             trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK ||
         fixed_header_->stream_frame_type == trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA);
  switch (fixed_header_->stream_frame_type) {
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT:
    return streamInitMeta_.ParseFromString(header_raw);
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE:
    return streamCloseMeta_.ParseFromString(header_raw);
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA:
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK:
    return true;
  default:
    PANIC("not reached");
  }
  return true;
}

void TrpcCodec::onCompleted(std::unique_ptr<Buffer::OwnedImpl> buffer) {
  origin_msg_ = std::move(buffer);
}

void TrpcCodec::toMetadata(MetaProtocolProxy::Metadata& metadata) {
  metadata.setHeaderSize(fixed_header_->getHeaderSize());
  metadata.setBodySize(fixed_header_->getPayloadSize());

  switch (fixed_header_->stream_frame_type) {
  case trpc::TrpcStreamFrameType::TRPC_UNARY:
    if (messageType_ == MetaProtocolProxy::MessageType::Request) {
      metadata.setRequestId(requestHeader_.request_id());
      metadata.putString("caller", requestHeader_.caller());
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
    break;
    // reset messageType of streaming messages to correct types
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_INIT:
    ENVOY_LOG(debug, "frame type: frame_init");
    metadata.setMessageType(MetaProtocolProxy::MessageType::Stream_Init);
    metadata.setStreamId(fixed_header_->stream_id);
    metadata.putString("caller", streamInitMeta_.request_meta().caller());
    metadata.putString("callee", streamInitMeta_.request_meta().callee());
    metadata.putString("func", streamInitMeta_.request_meta().func());
    for (auto const& kv : streamInitMeta_.request_meta().trans_info()) {
      metadata.putString(kv.first, kv.second);
    }
    break;
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_DATA:
    ENVOY_LOG(debug, "frame type: frame_data");
    metadata.setMessageType(MetaProtocolProxy::MessageType::Stream_Data);
    metadata.setStreamId(fixed_header_->stream_id);
    break;
    // Metaprotocol framework just treats feedback frame as a plain data frame
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_FEEDBACK:
    ENVOY_LOG(debug, "frame type: frame_feedback");
    metadata.setMessageType(MetaProtocolProxy::MessageType::Stream_Data);
    metadata.setStreamId(fixed_header_->stream_id);
    break;
  case trpc::TrpcStreamFrameType::TRPC_STREAM_FRAME_CLOSE:
    if (streamCloseMeta_.close_type() == trpc::TrpcStreamCloseType::TRPC_STREAM_CLOSE) {
      ENVOY_LOG(debug, "frame type: frame_close_one_way");
      metadata.setMessageType(MetaProtocolProxy::MessageType::Stream_Close_One_Way);
    } else {
      metadata.setMessageType(MetaProtocolProxy::MessageType::Stream_Close_Two_Way);
      ENVOY_LOG(debug, "frame type: frame_close_two_way");
    }
    metadata.setStreamId(fixed_header_->stream_id);
    break;
  }

  metadata.originMessage().move(*origin_msg_);
}

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

