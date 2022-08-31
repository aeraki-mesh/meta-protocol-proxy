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

  NOT_REACHED_GCOVR_EXCL_LINE;
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

  NOT_REACHED_GCOVR_EXCL_LINE;
}

static void annotateVerbose(Envoy::Tracing::Span& span, const StreamInfo::StreamInfo& stream_info) {
  const auto start_time = stream_info.startTime();
  if (stream_info.lastDownstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.lastDownstreamRxByteReceived()),
             Tracing::Logs::get().LastDownstreamRxByteReceived);
  }
  if (stream_info.firstUpstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.firstUpstreamTxByteSent()),
             Tracing::Logs::get().FirstUpstreamTxByteSent);
  }
  if (stream_info.lastUpstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.lastUpstreamTxByteSent()),
             Tracing::Logs::get().LastUpstreamTxByteSent);
  }
  if (stream_info.firstUpstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.firstUpstreamRxByteReceived()),
             Tracing::Logs::get().FirstUpstreamRxByteReceived);
  }
  if (stream_info.lastUpstreamRxByteReceived()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.lastUpstreamRxByteReceived()),
             Tracing::Logs::get().LastUpstreamRxByteReceived);
  }
  if (stream_info.firstDownstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.firstDownstreamTxByteSent()),
             Tracing::Logs::get().FirstDownstreamTxByteSent);
  }
  if (stream_info.lastDownstreamTxByteSent()) {
    span.log(start_time + std::chrono::duration_cast<SystemTime::duration>(
                              *stream_info.lastDownstreamTxByteSent()),
             Tracing::Logs::get().LastDownstreamTxByteSent);
  }
}

void MetaProtocolTracerUtility::finalizeSpanWithResponse(
    Envoy::Tracing::Span& span, const Metadata& response_metadata,
    const StreamInfo::StreamInfo& stream_info, const Envoy::Tracing::Config& tracing_config) {
  setCommonTags(span, stream_info, tracing_config);
  span.setTag(Tracing::Tags::get().ResponseSize,
              std::to_string(response_metadata.getHeaderSize() + response_metadata.getBodySize()));
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
  if (nullptr != stream_info.upstreamHost()) {
    span.setTag(Tracing::Tags::get().UpstreamCluster, stream_info.upstreamHost()->cluster().name());
    span.setTag(Tracing::Tags::get().UpstreamClusterName,
                stream_info.upstreamHost()->cluster().observabilityName());
  }

  if (tracing_config.verbose()) {
    annotateVerbose(span, stream_info);
  }
}

Envoy::Tracing::CustomTagConstSharedPtr
MetaProtocolTracerUtility::createCustomTag(const envoy::type::tracing::v3::CustomTag& tag) {
  switch (tag.type_case()) {
  case envoy::type::tracing::v3::CustomTag::TypeCase::kLiteral:
    return std::make_shared<const Tracing::LiteralCustomTag>(tag.tag(), tag.literal());
  case envoy::type::tracing::v3::CustomTag::TypeCase::kEnvironment:
    return std::make_shared<const Tracing::EnvironmentCustomTag>(tag.tag(), tag.environment());
  case envoy::type::tracing::v3::CustomTag::TypeCase::kRequestHeader:
    return std::make_shared<const Tracing::RequestHeaderCustomTag>(tag.tag(), tag.request_header());
  case envoy::type::tracing::v3::CustomTag::TypeCase::kMetadata:
    return std::make_shared<const Tracing::MetadataCustomTag>(tag.tag(), tag.metadata());
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
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
      std::string tag_key = "metadata: " + std::string(key.data(), key.size());
      active_span->setTag(tag_key, val);
      return true;
    });
  }

  return active_span;
}

void CustomTagBase::apply(Envoy::Tracing::Span& span,
                          const Envoy::Tracing::CustomTagContext& ctx) const {
  absl::string_view tag_value = value(ctx);
  if (!tag_value.empty()) {
    span.setTag(tag(), tag_value);
  }
}

EnvironmentCustomTag::EnvironmentCustomTag(
    const std::string& tag, const envoy::type::tracing::v3::CustomTag::Environment& environment)
    : CustomTagBase(tag), name_(environment.name()), default_value_(environment.default_value()) {
  const char* env = std::getenv(name_.data());
  final_value_ = env ? env : default_value_;
}

RequestHeaderCustomTag::RequestHeaderCustomTag(
    const std::string& tag, const envoy::type::tracing::v3::CustomTag::Header& request_header)
    : CustomTagBase(tag), name_(Http::LowerCaseString(request_header.name())),
      default_value_(request_header.default_value()) {}

absl::string_view RequestHeaderCustomTag::value(const Envoy::Tracing::CustomTagContext& ctx) const {
  if (ctx.trace_context == nullptr) {
    return default_value_;
  }
  // TODO(https://github.com/envoyproxy/envoy/issues/13454): Potentially populate all header values.
  const auto entry = ctx.trace_context->getByKey(name_);
  return entry.value_or(default_value_);
}

MetadataCustomTag::MetadataCustomTag(const std::string& tag,
                                     const envoy::type::tracing::v3::CustomTag::Metadata& metadata)
    : CustomTagBase(tag), kind_(metadata.kind().kind_case()),
      metadata_key_(metadata.metadata_key()), default_value_(metadata.default_value()) {}

void MetadataCustomTag::apply(Envoy::Tracing::Span& span,
                              const Envoy::Tracing::CustomTagContext& ctx) const {
  const envoy::config::core::v3::Metadata* meta = metadata(ctx);
  if (!meta) {
    if (!default_value_.empty()) {
      span.setTag(tag(), default_value_);
    }
    return;
  }
  const ProtobufWkt::Value& value = Envoy::Config::Metadata::metadataValue(meta, metadata_key_);
  switch (value.kind_case()) {
  case ProtobufWkt::Value::kBoolValue:
    span.setTag(tag(), value.bool_value() ? "true" : "false");
    return;
  case ProtobufWkt::Value::kNumberValue:
    span.setTag(tag(), absl::StrCat("", value.number_value()));
    return;
  case ProtobufWkt::Value::kStringValue:
    span.setTag(tag(), value.string_value());
    return;
  case ProtobufWkt::Value::kListValue:
    span.setTag(tag(), MessageUtil::getJsonStringFromMessageOrDie(value.list_value()));
    return;
  case ProtobufWkt::Value::kStructValue:
    span.setTag(tag(), MessageUtil::getJsonStringFromMessageOrDie(value.struct_value()));
    return;
  default:
    break;
  }
  if (!default_value_.empty()) {
    span.setTag(tag(), default_value_);
  }
}

const envoy::config::core::v3::Metadata*
MetadataCustomTag::metadata(const Envoy::Tracing::CustomTagContext& ctx) const {
  const StreamInfo::StreamInfo& info = ctx.stream_info;
  switch (kind_) {
  case envoy::type::metadata::v3::MetadataKind::KindCase::kRequest:
    return &info.dynamicMetadata();
  case envoy::type::metadata::v3::MetadataKind::KindCase::kRoute: {
    Router::RouteConstSharedPtr route = info.route();
    return route ? &route->metadata() : nullptr;
  }
  case envoy::type::metadata::v3::MetadataKind::KindCase::kCluster: {
    const auto& hostPtr = info.upstreamHost();
    return hostPtr ? &hostPtr->cluster().metadata() : nullptr;
  }
  case envoy::type::metadata::v3::MetadataKind::KindCase::kHost: {
    const auto& hostPtr = info.upstreamHost();
    return hostPtr ? hostPtr->metadata().get() : nullptr;
  }
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }
}

} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
