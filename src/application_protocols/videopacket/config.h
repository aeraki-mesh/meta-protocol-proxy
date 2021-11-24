#pragma once

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/videopacket/videopacket_codec.pb.h"
#include "src/application_protocols/videopacket/videopacket_codec.pb.validate.h"
#include "src/application_protocols/videopacket/video_packet.h"
#include "src/application_protocols/videopacket/videopacket_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace VideoPacket {

class VideoPacketCodecConfig 
  : public MetaProtocolProxy::CodecFactoryBase<aeraki::meta_protocol::codec::VideoPacketCodec> {
public:
  VideoPacketCodecConfig() : CodecFactoryBase("aeraki.meta_protocol.codec.videopacket") {}
  MetaProtocolProxy::CodecPtr createCodec(const Protobuf::Message& config) override;
};

}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy
