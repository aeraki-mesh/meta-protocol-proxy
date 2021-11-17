#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/trpc/trpc_codec.pb.h"
#include "src/application_protocols/trpc/trpc_codec.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

class TrpcCodecConfig
    : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::TrpcCodec> {
public:
  TrpcCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.trpc") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
