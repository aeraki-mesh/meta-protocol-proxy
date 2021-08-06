#include "envoy/registry/registry.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/application_protocols/thrift//config.h"
#include "src/application_protocols/thrift/thrift_codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Thrift {

MetaProtocolProxy::CodecPtr ThriftCodecConfig::createCodec(const Protobuf::Message&) {
  return std::make_unique<Thrift::ThriftCodec>();
};

/**
 * Static registration for the thrift codec. @see RegisterFactory.
 */
REGISTER_FACTORY(ThriftCodecConfig, MetaProtocolProxy::NamedCodecConfigFactory);

} // namespace Thrift
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
