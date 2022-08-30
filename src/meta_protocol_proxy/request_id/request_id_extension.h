#pragma once

#include <memory>
#include <string>

#include "envoy/common/pure.h"
#include "envoy/tracing/trace_reason.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

/**
 * Request ID functionality available via stream info.
 */
class RequestIdStreamInfoProvider {
public:
  virtual ~RequestIdStreamInfoProvider() = default;

  /**
   * Convert the request ID to a 64-bit integer representation for using in modulo, etc.
   * calculations.
   * @param request_metadata supplies the incoming request metadata for retrieving the request ID.
   * @return the integer or nullopt if the request ID is invalid.
   */
  virtual absl::optional<uint64_t> toInteger(const Metadata& request_metadata) const PURE;
};

using RequestIdStreamInfoProviderSharedPtr = std::shared_ptr<RequestIdStreamInfoProvider>;

/**
 * Abstract request id utilities for getting/setting the request IDs and tracing status of requests
 */
class RequestIDExtension : public RequestIdStreamInfoProvider {
public:
  /**
   * Directly set a request ID into the provided request metadata. Override any previous request ID
   * if any.
   * @param request_metadata supplies the incoming request metadata for setting a request ID.
   * @param force specifies if a new request ID should be forcefully set if one is already present.
   * @return true: the request ID in the request Metadata is modified
   */
  virtual bool set(Metadata& request_metadata, bool force) PURE;

  /**
   * Get the request ID from the request Metadata
   * @param request_metadata
   * @return request ID or "" if x-request-id header is not set
   */
  virtual std::string get(Metadata& request_metadata) PURE;

  /**
   * Preserve request ID in response headers if any is set in the request metadata.
   * @param response_headers supplies the downstream response headers for setting the request ID.
   * @param request_metadata supplies the incoming request metadata for retrieving the request ID.
   */
  // virtual void setInResponse(Http::ResponseHeaderMap& response_headers,
  //                            const Metadata& request_metadata) PURE;

  /**
   * Get the current tracing reason of a request given its headers.
   * @param request_metadata supplies the incoming request metadata for retrieving the request ID.
   * @return trace reason of the request based on the given headers.
   */
  virtual Envoy::Tracing::Reason getTraceReason(const Metadata& request_metadata) PURE;

  /**
   * Set the tracing status of a request.
   * @param request_metadata supplies the incoming request metadata for setting the trace reason.
   * @param status the trace reason that should be set for this request.
   */
  virtual void setTraceReason(Metadata& request_metadata, Envoy::Tracing::Reason reason) PURE;

  /**
   * Get whether to use request_id based sampling policy or not.
   * @return whether to use request_id based sampling policy or not.
   */
  virtual bool useRequestIdForTraceSampling() const PURE;
};

using RequestIDExtensionSharedPtr = std::shared_ptr<RequestIDExtension>;

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy