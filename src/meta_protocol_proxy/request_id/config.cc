#include "src/meta_protocol_proxy/request_id/config.h"

#include "source/common/common/random_generator.h"
#include "source/common/common/utility.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

bool UUIDRequestIDExtension::set(Metadata& request_metadata, bool force) {
  if (!force && request_metadata.getString(ReservedHeaders::RequestUUID) != "") {
    return false;
  }

  std::string uuid = random_.uuid();
  ASSERT(!uuid.empty());
  request_metadata.putString(ReservedHeaders::RequestUUID, uuid);
  return true;
}

std::string UUIDRequestIDExtension::get(Metadata& request_metadata) {
  return request_metadata.getString(ReservedHeaders::RequestUUID);
}

absl::optional<uint64_t> UUIDRequestIDExtension::toInteger(const Metadata& request_metadata) const {
  std::string uuid = request_metadata.getString(ReservedHeaders::RequestUUID);
  if (uuid == "") {
    return absl::nullopt;
  }

  if (uuid.length() < 8) {
    return absl::nullopt;
  }

  uint64_t value;
  if (!StringUtil::atoull(uuid.substr(0, 8).c_str(), value, 16)) {
    return absl::nullopt;
  }

  return value;
}

Tracing::Reason UUIDRequestIDExtension::getTraceReason(const Metadata& request_metadata) {
  std::string uuid = request_metadata.getString(ReservedHeaders::RequestUUID);
  if (uuid == "") {
    return Tracing::Reason::NotTraceable;
  }

  if (uuid.length() != Random::RandomGeneratorImpl::UUID_LENGTH) {
    return Tracing::Reason::NotTraceable;
  }

  switch (uuid[TRACE_BYTE_POSITION]) {
  case TRACE_FORCED:
    return Tracing::Reason::ServiceForced;
  case TRACE_SAMPLED:
    return Tracing::Reason::Sampling;
  case TRACE_CLIENT:
    return Tracing::Reason::ClientForced;
  default:
    return Tracing::Reason::NotTraceable;
  }
}

void UUIDRequestIDExtension::setTraceReason(Metadata& request_metadata, Tracing::Reason reason) {
  std::string uuid = request_metadata.getString(ReservedHeaders::RequestUUID);
  if (!pack_trace_reason_ || uuid == "") {
    return;
  }

  if (uuid.length() != Random::RandomGeneratorImpl::UUID_LENGTH) {
    return;
  }

  switch (reason) {
  case Tracing::Reason::ServiceForced:
    uuid[TRACE_BYTE_POSITION] = TRACE_FORCED;
    break;
  case Tracing::Reason::ClientForced:
    uuid[TRACE_BYTE_POSITION] = TRACE_CLIENT;
    break;
  case Tracing::Reason::Sampling:
    uuid[TRACE_BYTE_POSITION] = TRACE_SAMPLED;
    break;
  case Tracing::Reason::NotTraceable:
    uuid[TRACE_BYTE_POSITION] = NO_TRACE;
    break;
  default:
    break;
  }
  request_metadata.putString(ReservedHeaders::RequestUUID, uuid);
}

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
