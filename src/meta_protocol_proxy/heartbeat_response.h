#pragma once

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

struct HeartbeatResponse : public DirectResponse, Logger::Loggable<Logger::Id::filter> {
  HeartbeatResponse() = default;
  ~HeartbeatResponse() override = default;

  using ResponseType = DirectResponse::ResponseType;
  ResponseType encode(Metadata& metadata, Codec& codec, Buffer::Instance& buffer) const override;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
