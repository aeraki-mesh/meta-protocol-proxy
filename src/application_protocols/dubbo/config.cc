#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/dubbo/config.h"
#include "src/application_protocols/dubbo/dubbo_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

MetaProtocolProxy::CodecPtr DubboCodecConfig::createCodec(const Protobuf::Message&) {
  return std::make_unique<Dubbo::DubboCodec>();
};

/**
 * Static registration for the dubbo codec. @see RegisterFactory.
 */
REGISTER_FACTORY(DubboCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
