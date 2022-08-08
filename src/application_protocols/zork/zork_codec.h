#pragma once

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/zork/protocol.h"



namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Zork {

enum class ZorkDecodeStatus {
  DecodeHeader,
  DecodePayload,
  DecodeDone,
  WaitForData,
};



/**
 * Codec for Zork protocol.
 */
class ZorkCodec : public MetaProtocolProxy::Codec,
                  public Logger::Loggable<Logger::Id::misc> {
public:
  ZorkCodec(){};
  ~ZorkCodec() override = default;

  MetaProtocolProxy::DecodeStatus decode(Buffer::Instance& buffer,
                                         MetaProtocolProxy::Metadata& metadata) override;
  void encode(const MetaProtocolProxy::Metadata& metadata,
              const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) override;
  void onError(const MetaProtocolProxy::Metadata& metadata, const MetaProtocolProxy::Error& error,
               Buffer::Instance& buffer) override;


protected:
  ZorkDecodeStatus handleRequestState(Buffer::Instance& buffer);
  ZorkDecodeStatus handleResponseState(Buffer::Instance& buffer);

  ZorkDecodeStatus decodeRequestHeader(Buffer::Instance& buffer);
  ZorkDecodeStatus decodeRequestBody(Buffer::Instance& buffer);
  
  ZorkDecodeStatus decodeResponseHeader(Buffer::Instance& buffer);
  ZorkDecodeStatus decodeResponseBody(Buffer::Instance& buffer);
  void toMetadata(MetaProtocolProxy::Metadata& metadata);
  void toRespMetadata(MetaProtocolProxy::Metadata& metadata);

private:
  ZorkDecodeStatus decode_status_request{ZorkDecodeStatus::DecodeHeader};
  ZorkDecodeStatus decode_status_response{ZorkDecodeStatus::DecodeHeader};
  ZorkHeader zork_header_request_;
  ZorkHeader zork_header_response_;
  MetaProtocolProxy::MessageType messageType_;
  std::unique_ptr<Buffer::OwnedImpl> origin_msg_request_;
  std::unique_ptr<Buffer::OwnedImpl> origin_msg_response_;
};

} // namespace Zork
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
