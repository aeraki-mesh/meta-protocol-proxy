#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/brpc/config.h"
#include "src/application_protocols/brpc/brpc_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Brpc {

MetaProtocolProxy::CodecPtr BrpcCodecConfig::createCodec(const Protobuf::Message&) {
  return std::make_unique<Brpc::BrpcCodec>();
};

/**
 * Static registration for the trpc codec. @see RegisterFactory.
 */
REGISTER_FACTORY(BrpcCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

} // namespace Brpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
