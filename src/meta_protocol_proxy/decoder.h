#pragma once

#include "envoy/buffer/buffer.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/decoder_event_handler.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

#define ALL_PROTOCOL_STATES(FUNCTION)                                                              \
  FUNCTION(WaitForData)                                                                            \
  FUNCTION(OnDecodeStreamData)                                                                     \
  FUNCTION(Done)

/**
 * ProtocolState represents a set of states used in a state machine to decode requests and
 * responses.
 */
enum class ProtocolState { ALL_PROTOCOL_STATES(GENERATE_ENUM) };

class ProtocolStateNameValues {
public:
  static const std::string& name(ProtocolState state) {
    size_t i = static_cast<size_t>(state);
    ASSERT(i < names().size());
    return names()[i];
  }

private:
  static const std::vector<std::string>& names() {
    CONSTRUCT_ON_FIRST_USE(std::vector<std::string>, {ALL_PROTOCOL_STATES(GENERATE_STRING)});
  }
};

struct ActiveStream {
  ActiveStream(StreamHandler& handler, MetadataSharedPtr metadata, MutationSharedPtr mutation)
      : handler_(handler), metadata_(metadata), mutation_(mutation) {}
  ~ActiveStream() {
    metadata_.reset();
    mutation_.reset();
  }

  void onStreamDecoded() {
    ASSERT(metadata_ && mutation_);
    handler_.onStreamDecoded(metadata_, mutation_);
  }

  StreamHandler& handler_;
  MetadataSharedPtr metadata_;
  MutationSharedPtr mutation_;
};

using ActiveStreamPtr = std::unique_ptr<ActiveStream>;

class DecoderStateMachine : public Logger::Loggable<Logger::Id::filter> {
public:
  class Delegate {
  public:
    virtual ~Delegate() = default;
    virtual ActiveStream* newStream(MetadataSharedPtr metadata, MutationSharedPtr mutation) PURE;
    virtual void onHeartbeat(MetadataSharedPtr metadata) PURE;
  };

  DecoderStateMachine(Codec& codec, Delegate& delegate)
      : codec_(codec), delegate_(delegate), state_(ProtocolState::OnDecodeStreamData) {}

  /**
   * Consumes as much data from the configured Buffer as possible and executes the decoding state
   * machine. Returns ProtocolState::WaitForData if more data is required to complete processing of
   * a message. Returns ProtocolState::Done when the end of a message is successfully processed.
   * Once the Done state is reached, further invocations of run return immediately with Done.
   *
   * @param buffer a buffer containing the remaining data to be processed
   * @return ProtocolState returns with ProtocolState::WaitForData or ProtocolState::Done
   * @throw Envoy Exception if thrown by the underlying Protocol
   */
  ProtocolState run(Buffer::Instance& buffer);

  /**
   * @return the current ProtocolState
   */
  ProtocolState currentState() const { return state_; }

private:
  ProtocolState onDecodeStream(Buffer::Instance& buffer);

  Codec& codec_;
  Delegate& delegate_;
  ProtocolState state_;
};

using DecoderStateMachinePtr = std::unique_ptr<DecoderStateMachine>;

class DecoderBase : public DecoderStateMachine::Delegate,
                    public Logger::Loggable<Logger::Id::filter> {
public:
  DecoderBase(Codec& codec);
  ~DecoderBase() override;

  /**
   * Drains data from the given buffer
   *
   * @param data a Buffer containing protocol data
   * @throw EnvoyException on protocol errors
   */
  FilterStatus onData(Buffer::Instance& data, bool& buffer_underflow);

  // It is assumed that all of the protocol parsing are stateless,
  // if there is a state of the need to provide the reset interface call here.
  void reset();

protected:
  void start();
  void complete();

  Codec& codec_;
  ActiveStreamPtr stream_;
  DecoderStateMachinePtr state_machine_;

  bool decode_started_{false};
};

/**
 * Decoder encapsulates a configured and ProtocolPtr and SerializationPtr.
 */
template <typename T> class Decoder : public DecoderBase {
public:
  Decoder(Codec& codec, T& callbacks) : DecoderBase(codec), callbacks_(callbacks) {}

  ActiveStream* newStream(MetadataSharedPtr metadata, MutationSharedPtr mutation) override {
    ASSERT(!stream_);
    stream_ = std::make_unique<ActiveStream>(callbacks_.newStream(), metadata, mutation);
    return stream_.get();
  }

  void onHeartbeat(MetadataSharedPtr metadata) override { callbacks_.onHeartbeat(metadata); }

private:
  T& callbacks_;
};

class RequestDecoder : public Decoder<RequestDecoderCallbacks> {
public:
  RequestDecoder(Codec& codec, RequestDecoderCallbacks& callbacks) : Decoder(codec, callbacks) {}
};

using RequestDecoderPtr = std::unique_ptr<RequestDecoder>;

class ResponseDecoder : public Decoder<ResponseDecoderCallbacks> {
public:
  ResponseDecoder(Codec& codec, ResponseDecoderCallbacks& callbacks) : Decoder(codec, callbacks) {}
};

using ResponseDecoderPtr = std::unique_ptr<ResponseDecoder>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
