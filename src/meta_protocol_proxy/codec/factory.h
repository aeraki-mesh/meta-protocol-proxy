#pragma once

#include <memory>
#include <string>

#include "envoy/common/pure.h"

#include "common/protobuf/utility.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

/**
 * Implemented by each application protocol and registered via Registry::registerFactory or the
 * convenience class RegisterFactory.
 */
class NamedCodecConfigFactory : public Envoy::Config::TypedFactory {
public:
  ~NamedCodecConfigFactory() override = default;

  /**
   * Create a codec for a particular application protocol.
   * @param config the configuration of the codec.
   * @return protocol codec pointer.
   */
  virtual CodecPtr createCodec(const Protobuf::Message& config) PURE;

  std::string category() const override { return "aeraki.meta_protocol.codec"; }
};

template <class ConfigProto> class CodecFactoryBase : public NamedCodecConfigFactory {
public:
  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return std::make_unique<ConfigProto>();
  }

  std::string name() const override { return name_; }

protected:
  CodecFactoryBase(const std::string& name) : name_(name) {}

private:
  const std::string name_;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
