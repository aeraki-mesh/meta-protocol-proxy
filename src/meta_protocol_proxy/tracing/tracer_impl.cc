#include "src/meta_protocol_proxy/tracing/tracer_impl.h"
#include "src/meta_protocol_proxy/codec_impl.h"

#include <string>

#include "envoy/config/core/v3/base.pb.h"
#include "envoy/network/address.h"
#include "envoy/tracing/http_tracer.h"
#include "envoy/type/metadata/v3/metadata.pb.h"
#include "envoy/type/tracing/v3/custom_tag.pb.h"

#include "source/common/common/assert.h"
#include "source/common/common/fmt.h"
#include "source/common/common/macros.h"
#include "source/common/common/utility.h"
#include "source/common/formatter/substitution_formatter.h"
#include "source/common/grpc/common.h"
#include "source/common/http/codes.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/http/header_utility.h"
#include "source/common/http/headers.h"
#include "source/common/http/utility.h"
#include "source/common/protobuf/utility.h"
#include "source/common/stream_info/utility.h"

#include "absl/strings/str_cat.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

const std::string MetaProtocolTracerUtility::IngressOperation = "ingress";
const std::string MetaProtocolTracerUtility::EgressOperation = "egress";

const std::string tracing_headers[10] = {
    "x-request-id", "x-b3-traceid",  "x-b3-spanid",       "x-b3-parentspanid",
    "x-b3-sampled", "x-b3-flags",    "x-ot-span-context", "x-cloud-trace-context",
    "traceparent",  "grpc-trace-bin"};

const std::string&
MetaProtocolTracerUtility::toString(Envoy::Tracing::OperationName operation_name) {
  switch (operation_name) {
  case Envoy::Tracing::OperationName::Ingress:
    return IngressOperation;
  case Envoy::Tracing::OperationName::Egress:
    return EgressOperation;
  }

  PANIC("invalid operation name");
}

Envoy::Tracing::Decision
MetaProtocolTracerUtility::shouldTraceRequest(const StreamInfo::StreamInfo& stream_info) {
  // Exclude health check requests immediately.
  if (stream_info.healthCheck()) {
    return {Envoy::Tracing::Reason::HealthCheck, false};
  }

  const Envoy::Tracing::Reason trace_reason = stream_info.traceReason();
  switch (trace_reason) {
  case Envoy::Tracing::Reason::ClientForced:
  case Envoy::Tracing::Reason::ServiceForced:
  case Envoy::Tracing::Reason::Sampling:
    return {trace_reason, true};
  default:
    return {trace_reason, false};
  }

  PANIC("not reachable");
}

static void annotateVerbose(Envoy::Tracing::Span& span, const StreamInfo::StreamInfo& stream_info) {
  const auto start_time = stream_info.startTime();
  StreamInfo::TimingUtility timing(stream_info);
  if (timing.lastDownstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.lastDownstreamRxByteReceived()),
             Tracing::Logs::get().LastDownstreamRxByteReceived);
  }
  if (timing.firstUpstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.firstUpstreamTxByteSent()),
             Tracing::Logs::get().FirstUpstreamTxByteSent);
  }
  if (timing.lastUpstreamTxByteSent()) {
    span.log(start_time +
                 std::chrono::duration_cast<SystemTime::duration>(*timing.lastUpstreamTxByteSent()),
             Tracing::Logs::get().LastUpstreamTxByteSent);
  }
  if (timing.firstUpstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.firstUpstreamRxByteReceived()),
             Tracing::Logs::get().FirstUpstreamRxByteReceived);
  }
  if (timing.lastUpstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.lastUpstreamRxByteReceived()),
             Tracing::Logs::get().LastUpstreamRxByteReceived);
  }
  if (timing.firstDownstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.firstDownstreamTxByteSent()),
             Tracing::Logs::get().FirstDownstreamTxByteSent);
  }
  if (timing.lastDownstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *timing.lastDownstreamTxByteSent()),
             Tracing::Logs::get().LastDownstreamTxByteSent);
  }
}

void MetaProtocolTracerUtility::finalizeSpanWithResponse(
    Envoy::Tracing::Span& span, const Metadata& response_metadata,
    const StreamInfo::StreamInfo& stream_info, const Envoy::Tracing::Config& tracing_config) {
  setCommonTags(span, stream_info, tracing_config);
  span.setTag(Tracing::Tags::get().ResponseSize,
              std::to_string(response_metadata.getHeaderSize() + response_metadata.getBodySize()));
  response_metadata.forEach([&span](absl::string_view key, absl::string_view val) {
    std::string tag_key = "response metadata: " + std::string(key.data(), key.size());
    span.setTag(tag_key, val);
    return true;
  });
  if (response_metadata.getResponseStatus() == ResponseStatus::Error) {
    span.setTag(Tracing::Tags::get().Error, Tracing::Tags::get().True);
  }
  span.finishSpan();
}

void MetaProtocolTracerUtility::finalizeSpanWithoutResponse(
    Envoy::Tracing::Span& span, const StreamInfo::StreamInfo& stream_info,
    const Envoy::Tracing::Config& tracing_config, ResponseStatus response_status) {

  setCommonTags(span, stream_info, tracing_config);

  if (response_status == ResponseStatus::Error) {
    span.setTag(Tracing::Tags::get().Error, Tracing::Tags::get().True);
  }
  span.finishSpan();
}

void MetaProtocolTracerUtility::setCommonTags(Envoy::Tracing::Span& span,
                                              const StreamInfo::StreamInfo& stream_info,
                                              const Envoy::Tracing::Config& tracing_config) {
  const auto& remote_address = stream_info.downstreamAddressProvider().directRemoteAddress();

  if (remote_address->type() == Network::Address::Type::Ip) {
    const auto remote_ip = remote_address->ip();
    span.setTag(Tracing::Tags::get().PeerAddress, remote_ip->addressAsString());
  } else {
    span.setTag(Tracing::Tags::get().PeerAddress, remote_address->logicalName());
  }

  span.setTag(Tracing::Tags::get().Component, Tracing::Tags::get().Proxy);
  if (stream_info.upstreamInfo() && stream_info.upstreamInfo()->upstreamHost()) {
    span.setTag(Tracing::Tags::get().UpstreamCluster,
                stream_info.upstreamInfo()->upstreamHost()->cluster().name());
    span.setTag(Tracing::Tags::get().UpstreamClusterName,
                stream_info.upstreamInfo()->upstreamHost()->cluster().observabilityName());
  }

  if (tracing_config.verbose()) {
    annotateVerbose(span, stream_info);
  }
}

MetaProtocolTracerImpl::MetaProtocolTracerImpl(Envoy::Tracing::DriverSharedPtr driver,
                                               const LocalInfo::LocalInfo& local_info)
    : driver_(std::move(driver)), local_info_(local_info) {}

Envoy::Tracing::SpanPtr
MetaProtocolTracerImpl::startSpan(const Envoy::Tracing::Config& config, Metadata& request_metadata,
                                  Mutation& mutation, const StreamInfo::StreamInfo& stream_info,
                                  const std::string& cluster_name,
                                  const Envoy::Tracing::Decision tracing_decision) {

  std::string span_name = MetaProtocolTracerUtility::toString(Envoy::Tracing::OperationName());

  if (config.operationName() == Envoy::Tracing::OperationName::Egress) {
    span_name.append(" ");
    span_name.append(request_metadata.getOperationName());
  }

  Envoy::Tracing::SpanPtr active_span = driver_->startSpan(
      config, request_metadata, span_name, stream_info.startTime(), tracing_decision);

  // inject tracing context to metadata
  active_span->injectContext(request_metadata);
  // set tracing context related header to mutation so these headers can be sent to the application.
  // the application is responsible to pass these headers to upstream requests.
  for (int i = 0; i < 10; i++) {
    auto val = request_metadata.getString(tracing_headers[i]);
    if (val != "") {
      mutation[tracing_headers[i]] = val;
    }
  }

  // Set tags
  if (active_span) {
    auto xRequestId = request_metadata.getString(ReservedHeaders::RequestUUID);
    if (xRequestId != "") {
      active_span->setTag(Tracing::Tags::get().GuidXRequestId, xRequestId);
    }
    auto clientTraceId = request_metadata.getString(ReservedHeaders::ClientTraceId);
    if (clientTraceId != "") {
      active_span->setTag(Tracing::Tags::get().GuidXClientTraceId, clientTraceId);
    }
    active_span->setTag(Tracing::Tags::get().UpstreamClusterName, cluster_name);
    active_span->setTag(
        Tracing::Tags::get().RequestSize,
        std::to_string(request_metadata.getHeaderSize() + request_metadata.getBodySize()));
    active_span->setTag(Tracing::Tags::get().RequestID,
                        std::to_string(request_metadata.getRequestId()));
    active_span->setTag(Tracing::Tags::get().NodeId, local_info_.nodeName());
    active_span->setTag(Tracing::Tags::get().Zone, local_info_.zoneName());
    request_metadata.forEach([&active_span](absl::string_view key, absl::string_view val) {
      std::string tag_key = "request metadata: " + std::string(key.data(), key.size());
      active_span->setTag(tag_key, val);
      return true;
    });
  }

  return active_span;
}

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
