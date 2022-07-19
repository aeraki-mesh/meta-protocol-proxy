#pragma once

#include "src/application_protocols/dubbo/message_impl.h"
#include "src/application_protocols/dubbo/serializer.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

class DubboHessian2SerializerImpl : public Serializer {
public:
  const std::string& name() const override {
    return ProtocolSerializerNames::get().fromType(ProtocolType::Dubbo, type());
  }
  SerializationType type() const override { return SerializationType::Hessian2; }

  std::pair<RpcInvocationSharedPtr, bool>
  deserializeRpcInvocation(Buffer::Instance& buffer, ContextSharedPtr context) override;

  size_t serializeRpcInvocation(Buffer::Instance& output_buffer) override;

  std::pair<RpcResultSharedPtr, bool> deserializeRpcResult(Buffer::Instance& buffer,
                                                           ContextSharedPtr context) override;

  size_t serializeRpcResult(Buffer::Instance& output_buffer, const std::string& content,
                            RpcResponseType type) override;
};

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
