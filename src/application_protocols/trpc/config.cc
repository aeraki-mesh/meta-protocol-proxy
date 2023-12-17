#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/trpc/config.h"
#include "src/application_protocols/trpc/trpc_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

MetaProtocolProxy::CodecPtr TrpcCodecConfig::createCodec(const Protobuf::Message&) {
  return std::make_unique<Trpc::TrpcCodec>();
};

/**
 * Static registration for the trpc codec. @see RegisterFactory.
 */
REGISTER_FACTORY(TrpcCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
