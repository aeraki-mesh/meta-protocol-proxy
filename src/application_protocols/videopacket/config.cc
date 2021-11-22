#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/videopacket/config.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace VideoPacket {

    MetaProtocolProxy::CodecPtr VideoPacketCodecConfig::createCodec(const Protobuf::Message&) {
        return std::make_unique<VideoPacket::VideoPacketCodec>();
    };

   /**
   * Static registration for the videopacket codec. @see RegisterFactory.
   */
  REGISTER_FACTORY(VideoPacketCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy