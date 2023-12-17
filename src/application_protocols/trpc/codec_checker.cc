#include "src/application_protocols/trpc/codec_checker.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "source/common/common/assert.h"

#include "src/application_protocols/trpc/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

CodecChecker::DecodeStage CodecChecker::onData(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decoder onData: {}", buffer.length());

  while (decode_stage_ != DecodeStage::kDecodeDone) {
    auto state = handleState(buffer);
    if (state == DecodeStage::kWaitForData) {
      return DecodeStage::kWaitForData;
    }
    decode_stage_ = state;
  }

  ASSERT(decode_stage_ == DecodeStage::kDecodeDone);

  reset();
  ENVOY_LOG(debug, "trpc decoder: data length {}", buffer.length());
  return DecodeStage::kDecodeDone;
}

CodecChecker::DecodeStage CodecChecker::handleState(Buffer::Instance& buffer) {
  switch (decode_stage_) {
  case DecodeStage::kDecodeFixedHeader:
    return decodeFixedHeader(buffer);
  case DecodeStage::kDecodeUnaryProtocolHeader:
    return decodeUnaryProtocolHeader(buffer);
  case DecodeStage::KDecodeStreamFrame:
    return decodeStreamFrame(buffer);
  case DecodeStage::kDecodePayload:
    return decodePayload(buffer);
  default:
    PANIC("not reached");
  }
  return DecodeStage::kDecodeDone;
}

CodecChecker::DecodeStage CodecChecker::decodeFixedHeader(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decoder FixedHeader: {}", buffer.length());
  if (buffer.length() < TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE) {
    ENVOY_LOG(debug, "continue {}", buffer.length());
    return DecodeStage::kWaitForData;
  }

  std::unique_ptr<TrpcFixedHeader> fixed_header = std::make_unique<TrpcFixedHeader>();
  if (!fixed_header->decode(buffer, false)) {
    throw EnvoyException(fmt::format("protocol invalid"));
  }

  total_size_ = fixed_header->data_frame_size;
  protocol_header_size_ = fixed_header->pb_header_size;

  auto frame_type = fixed_header->stream_frame_type;
  call_backs_.onFixedHeaderDecoded(std::move(fixed_header));
  if (frame_type == trpc::TrpcStreamFrameType::TRPC_UNARY) {
    return DecodeStage::kDecodeUnaryProtocolHeader;
  }

  return DecodeStage::KDecodeStreamFrame;
}

CodecChecker::DecodeStage CodecChecker::decodeUnaryProtocolHeader(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decoder ProtocolHeader: {}", buffer.length());

  // 数据不全,继续收包
  if (buffer.length() < TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE + protocol_header_size_) {
    ENVOY_LOG(debug, "continue {}", buffer.length());
    return DecodeStage::kWaitForData;
  }
  std::string header_raw;
  header_raw.reserve(protocol_header_size_);
  header_raw.resize(protocol_header_size_);
  buffer.copyOut(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE, protocol_header_size_, &(header_raw[0]));

  if (!call_backs_.onUnaryHeader(std::move(header_raw))) {
    throw EnvoyException("parse header failed");
  }

  return DecodeStage::kDecodePayload;
}

CodecChecker::DecodeStage CodecChecker::decodeStreamFrame(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decoder stream frame {} ? {}", total_size_, buffer.length());

  if (buffer.length() < total_size_) {
    ENVOY_LOG(debug, "continue {}", buffer.length());
    return DecodeStage::kWaitForData;
  }

  std::string header_raw;
  auto frame_size = total_size_ - TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE;
  header_raw.reserve(frame_size);
  header_raw.resize(frame_size);
  buffer.copyOut(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE, frame_size, &(header_raw[0]));

  if (!call_backs_.onStreamFrame(std::move(header_raw))) {
    throw EnvoyException("parse header failed");
  }

  return DecodeStage::kDecodePayload;
}

CodecChecker::DecodeStage CodecChecker::decodePayload(Buffer::Instance& buffer) {
  ENVOY_LOG(debug, "decoder payload {} ? {}", total_size_, buffer.length());

  if (buffer.length() < total_size_) {
    ENVOY_LOG(debug, "continue {}", buffer.length());
    return DecodeStage::kWaitForData;
  }
  std::unique_ptr<Buffer::OwnedImpl> msg = std::make_unique<Buffer::OwnedImpl>();
  msg->move(buffer, static_cast<uint64_t>(total_size_));

  call_backs_.onCompleted(std::move(msg));

  return DecodeStage::kDecodeDone;
}

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
