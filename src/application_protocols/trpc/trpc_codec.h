#pragma once

#include <any>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/trpc/codec_checker.h"
#include "src/application_protocols/trpc/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

/**
 * Codec for Trpc protocol.
 */
class TrpcCodec : public MetaProtocolProxy::Codec,
                  public CodecCheckerCallBacks,
                  public Logger::Loggable<Logger::Id::misc> {
public:
  TrpcCodec() : decoder_base_(*this),messageType_(MetaProtocolProxy::MessageType::Request){};
  ~TrpcCodec() override = default;

  MetaProtocolProxy::DecodeStatus decode(Buffer::Instance& buffer,
                                         MetaProtocolProxy::Metadata& metadata) override;
  void encode(const MetaProtocolProxy::Metadata& metadata,
              const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) override;
  void onError(const MetaProtocolProxy::Metadata& metadata, const MetaProtocolProxy::Error& error,
               Buffer::Instance& buffer) override;
  void onFixedHeaderDecoded(std::unique_ptr<TrpcFixedHeader> fixed_header) override;
  bool onDecodeRequestOrResponseProtocol(std::string&& header_raw) override;
  void onCompleted(std::unique_ptr<Buffer::OwnedImpl> buffer) override;

private:
  void toMetadata(MetaProtocolProxy::Metadata& metadata);

private:
  CodecChecker decoder_base_;
  trpc::RequestProtocol requestHeader_;
  trpc::ResponseProtocol responseHeader_;
  std::unique_ptr<Buffer::OwnedImpl> origin_msg_;
  MetaProtocolProxy::MessageType messageType_;
};

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
