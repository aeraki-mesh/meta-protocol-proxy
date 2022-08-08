#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/zork/config.h"
#include "src/application_protocols/zork/zork_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Zork {

MetaProtocolProxy::CodecPtr ZorkCodecConfig::createCodec(const Protobuf::Message&) {
  return std::make_unique<Zork::ZorkCodec>();
};

/**
 * Static registration for the trpc codec. @see RegisterFactory.
 */
REGISTER_FACTORY(ZorkCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

} // namespace Zork
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
