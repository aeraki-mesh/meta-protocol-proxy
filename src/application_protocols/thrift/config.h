#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/thrift/thrift_codec.pb.h"
#include "src/application_protocols/thrift/thrift_codec.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Thrift {

class ThriftCodecConfig
    : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::ThriftCodec> {
public:
  ThriftCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.thrift") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

} // namespace Thrift
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
