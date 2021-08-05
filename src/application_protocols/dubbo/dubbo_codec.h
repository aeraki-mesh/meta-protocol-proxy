#pragma once

#include <any>
#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/common/optref.h"
#include "envoy/common/pure.h"

#include "source/common/common/logger.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/dubbo/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

/**
 * Codec for Dubbo protocol.
 */
class DubboCodec : public MetaProtocolProxy::Codec, public Logger::Loggable<Logger::Id::filter> {
public:
  DubboCodec() {
    protocol_ = NamedProtocolConfigFactory::getFactory(ProtocolType::Dubbo)
                    .createProtocol(SerializationType::Hessian2);
  };
  ~DubboCodec() override = default;

  MetaProtocolProxy::DecodeStatus decode(Buffer::Instance& buffer,
                                         MetaProtocolProxy::Metadata& metadata) override;
  void encode(const MetaProtocolProxy::Metadata& metadata,
              const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) override;
  void onError(const MetaProtocolProxy::Metadata& metadata, const MetaProtocolProxy::Error& error,
               Buffer::Instance& buffer) override;

private:
  void toMetadata(const MessageMetadata& msgMetadata, MetaProtocolProxy::Metadata& metadata);

  void toMetadata(const MessageMetadata& msgMetadata, Context& context,
                  MetaProtocolProxy::Metadata& metadata);
  void toMsgMetadata(const MetaProtocolProxy::Metadata& metadata, MessageMetadata& msgMetadata);

private:
  void encodeHeartbeat(const MetaProtocolProxy::Metadata& metadata, Buffer::Instance& buffer);

  ProtocolPtr protocol_;
};

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
