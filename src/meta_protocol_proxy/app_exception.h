#pragma once

#include "envoy/common/exception.h"

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

using ResponseType = DirectResponse::ResponseType;

template <typename T = ResponseStatus>
struct AppExceptionBase : public EnvoyException,
                          public DirectResponse,
                          Logger::Loggable<Logger::Id::filter> {
  AppExceptionBase(const AppExceptionBase& ex) = default;
  AppExceptionBase(const Error& error) : EnvoyException(error.message), error_(error) {}

  ResponseType encode(Metadata& metadata, Codec& codec, Buffer::Instance& buffer) const override {
    ASSERT(buffer.length() == 0);
    codec.onError(metadata, error_, buffer);
    return ResponseType::Exception;
  }

  const Error& error_;
};

using AppException = AppExceptionBase<>;

struct DownstreamConnectionCloseException : public EnvoyException {
  DownstreamConnectionCloseException(const std::string& what);
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
