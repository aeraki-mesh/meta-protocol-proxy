#pragma once

#include "src/application_protocols/dubbo/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

class DubboProtocolImpl : public Protocol {
public:
  DubboProtocolImpl() = default;
  ~DubboProtocolImpl() override = default;

  const std::string& name() const override { return ProtocolNames::get().fromType(type()); }
  ProtocolType type() const override { return ProtocolType::Dubbo; }

  std::pair<ContextSharedPtr, bool> decodeHeader(Buffer::Instance& buffer,
                                                 MessageMetadataSharedPtr metadata) override;
  bool decodeData(Buffer::Instance& buffer, ContextSharedPtr context,
                  MessageMetadataSharedPtr metadata) override;

  bool encode(Buffer::Instance& buffer, const MessageMetadata& metadata, const std::string& content,
              RpcResponseType type) override;

  static constexpr uint8_t MessageSize = 16;
  static constexpr int32_t MaxBodySize = 16 * 1024 * 1024;
};

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
