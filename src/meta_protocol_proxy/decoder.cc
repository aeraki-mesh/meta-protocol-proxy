#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

ProtocolState DecoderStateMachine::onDecodeStream(Buffer::Instance& buffer) {
  auto metadata = std::make_shared<MetadataImpl>();
  metadata->setMessageType(messageType_);
  auto decodeStatus = codec_.decode(buffer, *metadata);
  if (decodeStatus == DecodeStatus::WaitForData) {
    return ProtocolState::WaitForData;
  }

  if (metadata->getMessageType() == MessageType::Heartbeat) {
    ENVOY_LOG(debug, "meta protocol decoder: this is a heartbeat message");
    delegate_.onHeartbeat(metadata);
    return ProtocolState::Done;
  }

  auto mutation = std::make_shared<Mutation>();
  auto active_stream_ = delegate_.newStream(metadata, mutation);
  ASSERT(active_stream_);
  active_stream_->onStreamDecoded();
  return ProtocolState::Done;
}

ProtocolState DecoderStateMachine::run(Buffer::Instance& buffer) {
  ASSERT(state_ != ProtocolState::Done);
  ENVOY_LOG(trace, "meta protocol decoder: state {}, {} bytes available",
            ProtocolStateNameValues::name(state_), buffer.length());
  state_ = onDecodeStream(buffer);
  return state_;
}

using DecoderStateMachinePtr = std::unique_ptr<DecoderStateMachine>;

DecoderBase::DecoderBase(Codec& codec, MessageType messageType)
    : codec_(codec), messageType_(messageType) {}

DecoderBase::~DecoderBase() { complete(); }

void DecoderBase::onData(Buffer::Instance& data, bool& buffer_underflow) {
  ENVOY_LOG(debug, "MetaProtocol decoder: {} bytes available", data.length());
  buffer_underflow = false;

  // Start to decode a message
  if (!decode_started_) {
    start();
  }
  ASSERT(state_machine_ != nullptr);

  ENVOY_LOG(debug, "MetaProtocol decoder: state {}, {} bytes available",
            ProtocolStateNameValues::name(state_machine_->currentState()), data.length());

  ProtocolState state = state_machine_->run(data);
  switch (state) {
  case ProtocolState::WaitForData:
    ENVOY_LOG(debug, "MetaProtocol decoder: wait for data");
    // set buffer_underflow as true if we need more data to complete decoding of the current message
    buffer_underflow = true;
    return;
  default:
    break;
  }

  ASSERT(state == ProtocolState::Done);

  // Clean up after finishing decoding a message
  complete();
  // important: set buffer_underflow as true if all data in the current buffer has been processed to
  // break the outer dispatching loop
  buffer_underflow = (data.length() == 0);
  ENVOY_LOG(debug, "MetaProtocol decoder: data length {}", data.length());
  return;
}

/**
 * Start to decode a message
 */
void DecoderBase::start() {
  state_machine_ = std::make_unique<DecoderStateMachine>(codec_, messageType_, *this);
  decode_started_ = true;
}

/**
 * Finishing decoding a message
 */
void DecoderBase::complete() {
  state_machine_.reset();
  stream_.reset();
  decode_started_ = false;
}

void DecoderBase::reset() { complete(); }

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
