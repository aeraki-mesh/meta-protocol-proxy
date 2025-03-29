#include "src/application_protocols/dubbo/dubbo_hessian2_serializer_impl.h"

#include "envoy/common/exception.h"

#include "source/common/common/assert.h"
#include "source/common/common/macros.h"
#include "src/application_protocols/dubbo/hessian_utils.h"
#include "src/application_protocols/dubbo/message_impl.h"

#include "hessian2/object.hpp"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Dubbo {

std::pair<RpcInvocationSharedPtr, bool>

DubboHessian2SerializerImpl::deserializeRpcInvocation(Buffer::Instance& buffer,
                                                      ContextSharedPtr context) {
  Hessian2::Decoder decoder(std::make_unique<BufferReader>(buffer));

  // TODO(zyfjeff): Add format checker
  auto dubbo_version = decoder.decode<std::string>();
  auto service_name = decoder.decode<std::string>();
  auto service_version = decoder.decode<std::string>();
  auto method_name = decoder.decode<std::string>();

  if (context->bodySize() < decoder.offset()) {
    throw EnvoyException(fmt::format("RpcInvocation size({}) larger than body size({})",
                                     decoder.offset(), context->bodySize()));
  }

  if (dubbo_version == nullptr || service_name == nullptr || service_version == nullptr ||
      method_name == nullptr) {
    throw EnvoyException(fmt::format("RpcInvocation has no request metadata"));
  }

  auto invo = std::make_shared<RpcInvocationImpl>();
  invo->setServiceName(*service_name);
  invo->setServiceVersion(*service_version);
  invo->setMethodName(*method_name);

  size_t parsed_size = context->headerSize() + decoder.offset();

  auto delayed_decoder = std::make_shared<Hessian2::Decoder>(
      std::make_unique<BufferReader>(context->originMessage(), parsed_size));

  invo->setParametersLazyCallback([delayed_decoder]() -> RpcInvocationImpl::ParametersPtr {
    auto params = std::make_unique<RpcInvocationImpl::Parameters>();

    if (auto types = delayed_decoder->decode<std::string>(); types != nullptr && !types->empty()) {
      uint32_t number = HessianUtils::getParametersNumber(*types);
      for (uint32_t i = 0; i < number; i++) {
        if (auto result = delayed_decoder->decode<Hessian2::Object>(); result != nullptr) {
          params->push_back(std::move(result));
        } else {
          throw EnvoyException("Cannot parse RpcInvocation parameter from buffer");
        }
      }
    }
    return params;
  });

  invo->setAttachmentLazyCallback([delayed_decoder]() -> RpcInvocationImpl::AttachmentPtr {
    size_t offset = delayed_decoder->offset();

    auto result = delayed_decoder->decode<Hessian2::Object>();
    if (result != nullptr && result->type() == Hessian2::Object::Type::UntypedMap) {
      return std::make_unique<RpcInvocationImpl::Attachment>(
          RpcInvocationImpl::Attachment::MapPtr{
              dynamic_cast<RpcInvocationImpl::Attachment::Map*>(result.release())},
          offset);
    } else {
      return std::make_unique<RpcInvocationImpl::Attachment>(
          std::make_unique<RpcInvocationImpl::Attachment::Map>(), offset);
    }
  });

  return std::pair<RpcInvocationSharedPtr, bool>(invo, true);
}

size_t DubboHessian2SerializerImpl::serializeRpcInvocation(Buffer::Instance& output_buffer) {
  // todo headerMutation
  return output_buffer.length();
}

std::pair<RpcResultSharedPtr, bool>
DubboHessian2SerializerImpl::deserializeRpcResult(Buffer::Instance& buffer,
                                                  ContextSharedPtr context) {
  ASSERT(buffer.length() >= context->bodySize());
  bool has_value_or_attachment = true;

  auto result = std::make_shared<RpcResultImpl>();

  Hessian2::Decoder decoder(std::make_unique<BufferReader>(buffer));
  auto type_value = decoder.decode<int32_t>();
  if (type_value == nullptr) {
    throw EnvoyException(fmt::format("Cannot parse RpcResult type from buffer"));
  }

  RpcResponseType type = static_cast<RpcResponseType>(*type_value);

  bool has_attachment = false;
  bool has_value = false;
  switch (type) {
  case RpcResponseType::ResponseWithException:
    has_value_or_attachment = false;
    result->setException(true);
    break;
  case RpcResponseType::ResponseWithExceptionWithAttachments:
    has_value_or_attachment = false;
    result->setException(true);
    has_attachment = true;
    break;
  case RpcResponseType::ResponseWithNullValue:
    has_value_or_attachment = false;
    has_value = false;
    has_attachment = false;
    FALLTHRU;
  case RpcResponseType::ResponseNullValueWithAttachments:
    has_value = false;
    has_attachment = true;
    result->setException(false);
    break;
  case RpcResponseType::ResponseWithValue:
    has_value = true;
    has_attachment = false;
    result->setException(false);
    break;
  case RpcResponseType::ResponseValueWithAttachments:
    has_value = true;
    has_attachment = true;
    result->setException(false);
    break;
  default:
    throw EnvoyException(fmt::format("not supported return type {}", static_cast<uint8_t>(type)));
  }

  size_t total_size = decoder.offset();
  // decode  response body
  if (has_value) {
    auto responesebody = decoder.decode<std::string>();
    result->setRspBody(*responesebody);
  }

  if (has_attachment) {
    size_t offset = context->headerSize() + decoder.offset();
    auto attachResult = decoder.decode<Hessian2::Object>();
    if (attachResult != nullptr && attachResult->type() == Hessian2::Object::Type::UntypedMap) {
      result->attachment_ = std::make_unique<RpcInvocationImpl::Attachment>(
          RpcInvocationImpl::Attachment::MapPtr{
              dynamic_cast<RpcInvocationImpl::Attachment::Map*>(attachResult.release())},
          offset);
    } else {
      result->attachment_ = std::make_unique<RpcInvocationImpl::Attachment>(
          std::make_unique<RpcInvocationImpl::Attachment::Map>(), offset);
    }
  }

  if (context->bodySize() < total_size) {
    throw EnvoyException(fmt::format("RpcResult size({}) large than body size({})", total_size,
                                     context->bodySize()));
  }

  if (!has_value_or_attachment && context->bodySize() != total_size) {
    throw EnvoyException(
        fmt::format("RpcResult is no value, but the rest of the body size({}) not equal 0",
                    (context->bodySize() - total_size)));
  }

  return std::pair<RpcResultSharedPtr, bool>(result, true);
}

size_t DubboHessian2SerializerImpl::serializeRpcResult(Buffer::Instance& output_buffer,
                                                       const std::string& content,
                                                       RpcResponseType type) {
  size_t origin_length = output_buffer.length();
  Hessian2::Encoder encoder(std::make_unique<BufferWriter>(output_buffer));

  // The serialized response type is compact int.
  bool result = encoder.encode(static_cast<std::underlying_type<RpcResponseType>::type>(type));
  result |= encoder.encode(content);

  ASSERT(result);

  return output_buffer.length() - origin_length;
}

class DubboHessian2SerializerConfigFactory
    : public SerializerFactoryBase<DubboHessian2SerializerImpl> {
public:
  DubboHessian2SerializerConfigFactory()
      : SerializerFactoryBase(ProtocolType::Dubbo, SerializationType::Hessian2) {}
};

/**
 * Static registration for the Hessian protocol. @see RegisterFactory.
 */
REGISTER_FACTORY(DubboHessian2SerializerConfigFactory, NamedSerializerConfigFactory);

} // namespace Dubbo
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
