#pragma once

// #include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/videopacket/protocol.h"
#include "src/application_protocols/videopacket/video_packet.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace VideoPacket {

class VideoPacketCodecConfig
    : public ProtocolFactoryBase<CVideoPacket> {
public:
  VideoPacketCodecConfig() : ProtocolFactoryBase("aeraki.meta_protocol.codec.videopacket") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy
