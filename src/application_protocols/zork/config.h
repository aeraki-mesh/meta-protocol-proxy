#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/zork/zork_codec.pb.h"
#include "src/application_protocols/zork/zork_codec.pb.validate.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Zork {

class ZorkCodecConfig
    : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::ZorkCodec> {
public:
  ZorkCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.zork") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

} // namespace Zork
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
