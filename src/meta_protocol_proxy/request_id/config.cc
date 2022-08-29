#include "src/meta_protocol_proxy/request_id/config.h"

#include "source/common/common/random_generator.h"
#include "source/common/common/utility.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

std::string UUIDRequestIDExtension::getXRequestID(const Metadata& metadata) const {
  return metadata.getString(X_REQUEST_ID);
}
void UUIDRequestIDExtension::setXRequestID(std::string x_request_id,
                                                  Metadata& metadata) const {
  metadata.putString(X_REQUEST_ID, x_request_id);
}

void UUIDRequestIDExtension::set(Metadata& request_metadata, bool force) {
  if (!force && getXRequestID(request_metadata) != "") {
    return;
  }

  std::string uuid = random_.uuid();
  ASSERT(!uuid.empty());
  setXRequestID(uuid, request_metadata);
}

absl::optional<uint64_t> UUIDRequestIDExtension::toInteger(const Metadata& request_metadata) const {
  std::string uuid = getXRequestID(request_metadata);
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
  std::string uuid = getXRequestID(request_metadata);
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
  std::string uuid = getXRequestID(request_metadata);
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
  setXRequestID(uuid, request_metadata);
}

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
