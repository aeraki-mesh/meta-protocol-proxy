#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/brpc/brpc_codec.pb.h"
#include "src/application_protocols/brpc/brpc_codec.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Brpc {

class BrpcCodecConfig
    : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::BrpcCodec> {
public:
  BrpcCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.brpc") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

} // namespace Brpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
