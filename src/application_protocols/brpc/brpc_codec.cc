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
  ENVOY_LOG(debug, "Brpc decoder: {} bytes available, msg type: {}", buffer.length(),
            static_cast<int>(metadata.getMessageType()));
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

void BrpcCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  ASSERT(buffer.length() == 0);
  BrpcHeader response;
  // Make sure to set the request id if the application protocol has one, otherwise MetaProtocol
  // framework will
  // complaint that the id in the error response is not equal to the one in the request

  (void)metadata;
  //response.set_pack_flow(metadata.getRequestId());
  int16_t error_code = 0; // just for rsp error code
  int brpc_error_code = 0;
  std::string error_msg = "from_mesh:" + error.message;
  switch (error.type) {
  case MetaProtocolProxy::ErrorType::RouteNotFound:
    error_code = static_cast<int16_t>(BrpcCode::NoRoute);
    brpc_error_code = 1019; // EROUTE
    break;
  case MetaProtocolProxy::ErrorType::ClusterNotFound:
    error_code = static_cast<int16_t>(BrpcCode::NoCluster);
    brpc_error_code = 1001; // ENOSERVICE
    break;
  case MetaProtocolProxy::ErrorType::NoHealthyUpstream:
    error_code = static_cast<int16_t>(BrpcCode::UnHealthy);
    brpc_error_code = 2005; // ECLOSE
    break;
  case MetaProtocolProxy::ErrorType::BadResponse:
    error_code = static_cast<int16_t>(BrpcCode::BadResponse);
    brpc_error_code = 2002; // ERESPONSE
    break;
  case MetaProtocolProxy::ErrorType::Unspecified:
    error_code = static_cast<int16_t>(BrpcCode::UnspecifiedError);
    brpc_error_code = 2001; // EINTERNAL
    break;
  case MetaProtocolProxy::ErrorType::OverLimit:
    error_code = static_cast<int16_t>(BrpcCode::OverLimit);
    brpc_error_code = 2004; // ELIMIT
    break;
  default:
    error_code = static_cast<int16_t>(BrpcCode::Error);
    brpc_error_code = 2001; // EINTERNAL
    break;
  }

  //response.set_rsp_code(error_code);

  aeraki::meta_protocol::brpc::RpcMeta meta;
  aeraki::meta_protocol::brpc::RpcResponseMeta* response_meta = meta.mutable_response();
  response_meta->set_error_code(brpc_error_code);
  response_meta->set_error_text(error_msg);
  //response_meta->set_load_baalancer_code(?);
  meta.set_correlation_id(meta_.correlation_id());
  meta.set_compress_type(meta_.compress_type());

  const int meta_size = meta.ByteSize();
  //response.set_pack_len(BrpcHeader::HEADER_SIZE + meta_size);
  int payload_size = 0;
  response.set_body_len(meta_size + payload_size);
  response.set_meta_len(meta_size);
  response.encode(buffer);

  char* meta_buffer = NULL;
  char buffer_local[1024];
  if (meta_size < 1024)
  {
    meta_buffer = buffer_local;
  }
  else
  {
    meta_buffer = new char[meta_size];
  }

  ::google::protobuf::io::ArrayOutputStream arr_out(meta_buffer, meta_size);
  ::google::protobuf::io::CodedOutputStream coded_out(&arr_out);
  meta.SerializeWithCachedSizes(&coded_out);
  ASSERT(!coded_out.HadError());
  buffer.add(meta_buffer, meta_size);

  ENVOY_LOG(warn, "brpc onError error={} buffer_len={} detail={}",
    static_cast<int>(error.type), static_cast<int>(buffer.length()),
    meta.ShortDebugString());

  if (meta_size < 1024)
  {
    meta_buffer = NULL;
  }
  else
  {
    delete []meta_buffer;
    meta_buffer = NULL;
  }

  /*
  if (span && received_us > 0)
  {
    response_meta->set_process_time_us(base::cpuwide_time_us() - received_us);
  }
  */
}

BrpcDecodeStatus BrpcCodec::handleState(Buffer::Instance& buffer) {
  switch (decode_status) {
  case BrpcDecodeStatus::DecodeHeader:
    return decodeHeader(buffer);
  case BrpcDecodeStatus::DecodePayload:
    return decodeBody(buffer);
  default:
    PANIC("not reached");
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

  if (!brpc_header_.decode(buffer)) {
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
  metadata.getOriginMessage().move(*origin_msg_);
}

} // namespace Brpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy