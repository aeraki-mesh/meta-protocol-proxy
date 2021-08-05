#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/dubbo/dubbo_codec.pb.h"
#include "src/application_protocols/dubbo/dubbo_codec.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

class DubboCodecConfig
    : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::DubboCodec> {
public:
  DubboCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.dubbo") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
