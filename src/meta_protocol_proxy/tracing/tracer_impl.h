#pragma once

#include <string>

#include "envoy/common/platform.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/http/request_id_extension.h"
#include "envoy/local_info/local_info.h"
#include "envoy/type/metadata/v3/metadata.pb.h"
#include "envoy/type/tracing/v3/custom_tag.pb.h"
#include "envoy/upstream/cluster_manager.h"

#include "source/common/config/metadata.h"
#include "source/common/json/json_loader.h"

#include "src/meta_protocol_proxy/tracing/common_values.h"
#include "src/meta_protocol_proxy/tracing/null_span_impl.h"
#include "src/meta_protocol_proxy/tracing/tracer.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Tracing {

class MetaProtocolTracerUtility {
public:
  /**
   * Get string representation of the operation.
   * @param operation name to convert.
   * @return string representation of the operation.
   */
  static const std::string& toString(Envoy::Tracing::OperationName operation_name);

  /**
   * Request might be traceable if the request ID is traceable or we do sampling tracing.
   * Note: there is a global switch which turns off tracing completely on server side.
   *
   * @return decision if request is traceable or not and Reason why.
   **/
  static Envoy::Tracing::Decision shouldTraceRequest(const StreamInfo::StreamInfo& stream_info);

  /**
   * Adds information obtained from the downstream request headers as tags to the active span.
   * Then finishes the span.
   */
  static void finalizeSpanWithResponse(Envoy::Tracing::Span& span,
                                       const Metadata& response_metadata,
                                       const StreamInfo::StreamInfo& stream_info,
                                       const Envoy::Tracing::Config& tracing_config);

  /**
   * Adds information obtained from the upstream request headers as tags to the active span.
   * Then finishes the span.
   */
  static void finalizeSpanWithoutResponse(Envoy::Tracing::Span& span,
                                          const StreamInfo::StreamInfo& stream_info,
                                          const Envoy::Tracing::Config& tracing_config,
                                          ResponseStatus response_status);

  /**
   * Create a custom tag according to the configuration.
   * @param tag a tracing custom tag configuration.
   */
  //static Envoy::Tracing::CustomTagConstSharedPtr
  //createCustomTag(const envoy::type::tracing::v3::CustomTag& tag);

private:
  static void setCommonTags(Envoy::Tracing::Span& span,
                            // const Http::ResponseHeaderMap* response_headers,
                            // const Http::ResponseTrailerMap* response_trailers,
                            const StreamInfo::StreamInfo& stream_info,
                            const Envoy::Tracing::Config& tracing_config);

  static const std::string IngressOperation;
  static const std::string EgressOperation;
};

class EgressConfigImpl : public Envoy::Tracing::Config {
public:
  // Tracing::Config
  Envoy::Tracing::OperationName operationName() const override {
    return Envoy::Tracing::OperationName::Egress;
  }
  const Envoy::Tracing::CustomTagMap* customTags() const override { return nullptr; }
  bool verbose() const override { return false; }
  uint32_t maxPathTagLength() const override { return Envoy::Tracing::DefaultMaxPathTagLength; }
};

using EgressConfig = ConstSingleton<EgressConfigImpl>;

class NullTracer : public MetaProtocolTracer {
public:
  // Tracing::MetaProtocolTracer
  Envoy::Tracing::SpanPtr startSpan(const Envoy::Tracing::Config&, Metadata&, Mutation&,
                                    const StreamInfo::StreamInfo&, const std::string&,
                                    const Envoy::Tracing::Decision) override {
    return Envoy::Tracing::SpanPtr{new NullSpan()};
  }
};

class MetaProtocolTracerImpl : public MetaProtocolTracer, Logger::Loggable<Logger::Id::filter> {
public:
  MetaProtocolTracerImpl(Envoy::Tracing::DriverSharedPtr driver,
                         const LocalInfo::LocalInfo& local_info);

  // Tracing::MetaProtocolTracer
  Envoy::Tracing::SpanPtr startSpan(const Envoy::Tracing::Config& config, Metadata& metadata,
                                    Mutation& mutation, const StreamInfo::StreamInfo& stream_info,
                                    const std::string& cluster_name,
                                    const Envoy::Tracing::Decision tracing_decision) override;

  Envoy::Tracing::DriverSharedPtr driverForTest() const { return driver_; }

private:
  Envoy::Tracing::DriverSharedPtr driver_;
  const LocalInfo::LocalInfo& local_info_;
};

/**
 * Configuration for tracing which is set on the MetaProtocol Proxy level.
 * Tracing can be enabled/disabled on a per MetaProtocol Proxy basis.
 * Here we specify some specific for MetaProtocol Proxy settings.
 */
class TracingConfigImpl : public Tracing::TracingConfig {
public:
  TracingConfigImpl(Envoy::Tracing::OperationName operation_name,
                    envoy::type::v3::FractionalPercent client_sampling,
                    envoy::type::v3::FractionalPercent random_sampling,
                    envoy::type::v3::FractionalPercent overall_sampling, bool verbose,
                    int32_t max_tag_length)
      : operation_name_(operation_name), client_sampling_(client_sampling),
        random_sampling_(random_sampling), overall_sampling_(overall_sampling), verbose_(verbose),
        max_tag_length_(max_tag_length) {
    custom_tags_ = std::make_unique<Envoy::Tracing::CustomTagMap>();
  }
  Envoy::Tracing::OperationName operationName() const override { return operation_name_; };
  bool spawnUpstreamSpan() const override { return true; };
  const Envoy::Tracing::CustomTagMap* customTags() const override { return custom_tags_.get(); };
  bool verbose() const override { return verbose_; };
  uint32_t maxPathTagLength() const override { return max_tag_length_; };
  envoy::type::v3::FractionalPercent& clientSampling() override { return client_sampling_; };
  envoy::type::v3::FractionalPercent& randomSampling() override { return random_sampling_; };
  envoy::type::v3::FractionalPercent& overallSampling() override { return overall_sampling_; };

private:
  Envoy::Tracing::OperationName operation_name_;
  std::unique_ptr<Envoy::Tracing::CustomTagMap> custom_tags_;
  envoy::type::v3::FractionalPercent client_sampling_;
  envoy::type::v3::FractionalPercent random_sampling_;
  envoy::type::v3::FractionalPercent overall_sampling_;
  bool verbose_;
  uint32_t max_tag_length_;
};
using TracingConfigSharedPtr = std::shared_ptr<TracingConfig>;
} // namespace Tracing
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy