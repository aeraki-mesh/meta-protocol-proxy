#pragma once

#include "envoy/common/random_generator.h"

#include "src/meta_protocol_proxy/request_id/request_id_extension.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

// UUIDRequestIDExtension is the default implementation if no other extension is explicitly
// configured.
class UUIDRequestIDExtension : public RequestIDExtension {
public:
  UUIDRequestIDExtension(Random::RandomGenerator& random) : random_(random) {}

  static RequestIDExtensionSharedPtr defaultInstance(Random::RandomGenerator& random) {
    return std::make_shared<UUIDRequestIDExtension>(random);
  }

  bool packTraceReason() { return pack_trace_reason_; }

  // Http::RequestIDExtension
  bool set(Metadata& request_metadata, bool force) override;
  std::string get(Metadata& request_metadata) override;
  absl::optional<uint64_t> toInteger(const Metadata& request_metadata) const override;
  Envoy::Tracing::Reason getTraceReason(const Metadata& request_metadata) override;
  void setTraceReason(Metadata& request_metadata, Envoy::Tracing::Reason status) override;
  bool useRequestIdForTraceSampling() const override { return use_request_id_for_trace_sampling_; }

private:
  Envoy::Random::RandomGenerator& random_;
  const bool pack_trace_reason_ = true;
  const bool use_request_id_for_trace_sampling_ = true;

  // Byte on this position has predefined value of 4 for UUID4.
  static const int TRACE_BYTE_POSITION = 14;

  // Value of '9' is chosen randomly to distinguish between freshly generated uuid4 and the
  // one modified because we sample trace.
  static const char TRACE_SAMPLED = '9';

  // Value of 'a' is chosen randomly to distinguish between freshly generated uuid4 and the
  // one modified because we force trace.
  static const char TRACE_FORCED = 'a';

  // Value of 'b' is chosen randomly to distinguish between freshly generated uuid4 and the
  // one modified because of client trace.
  static const char TRACE_CLIENT = 'b';

  // Initial value for freshly generated uuid4.
  static const char NO_TRACE = '4';
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
