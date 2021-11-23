#pragma once

#include "src/meta_protocol_proxy/codec/codec.h"
#include "envoy/config/typed_config.h"
#include "envoy/common/pure.h"
#include "src/application_protocols/videopacket/video_packet.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace VideoPacket {

using VideoPacketPtr = std::unique_ptr<CVideoPacket>;

class NamedProtocolConfigFactory : public Config::UntypedFactory {
public:
  ~NamedProtocolConfigFactory() override = default;

  // unused
  virtual VideoPacketPtr createProtocol() PURE;

  /**
   * Create a codec for a particular application protocol.
   * @param config the configuration of the codec.
   * @return protocol codec pointer.
   */
  virtual CodecPtr createCodec(const Protobuf::Message& config) PURE;

  std::string category() const override { return "envoy.videopacket_proxy.protocols"; }
};

template <class ProtoImpl> class ProtocolFactoryBase : public NamedProtocolConfigFactory {
public:
  VideoPacketPtr createProtocol() override {
    return std::make_unique<ProtoImpl>();
  }

  std::string name() const override { return name_; }

protected:
  explicit ProtocolFactoryBase(const std::string& name) : name_(name) {}

private:
  const std::string name_;
};


}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy

