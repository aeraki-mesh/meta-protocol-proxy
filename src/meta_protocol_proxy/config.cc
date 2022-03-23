#include "src/meta_protocol_proxy/config.h"

#include "absl/container/flat_hash_map.h"

#include "envoy/registry/registry.h"
#include "envoy/tracing/http_tracer.h"
#include "common/config/utility.h"
#include "common/tracing/http_tracer_config_impl.h"
#include "common/tracing/http_tracer_manager_impl.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"
#include "src/meta_protocol_proxy/stats.h"
#include "src/meta_protocol_proxy/route/route_config_provider_manager.h"
#include "src/meta_protocol_proxy/route/rds_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

// Singleton registration via macro defined in envoy/singleton/manager.h
SINGLETON_MANAGER_REGISTRATION(meta_protocol_route_config_provider_manager);
SINGLETON_MANAGER_REGISTRATION(meta_protocol_tracer_manager);

Utility::Singletons Utility::createSingletons(Server::Configuration::FactoryContext& context) {
  Route::RouteConfigProviderManagerSharedPtr meta_route_config_provider_manager =
      context.singletonManager().getTyped<Route::RouteConfigProviderManager>(
          SINGLETON_MANAGER_REGISTERED_NAME(meta_protocol_route_config_provider_manager),
          [&context] {
            return std::make_shared<Route::RouteConfigProviderManagerImpl>(context.admin());
          });

  auto http_tracer_manager = context.singletonManager().getTyped<Tracing::HttpTracerManagerImpl>(
      SINGLETON_MANAGER_REGISTERED_NAME(meta_protocol_tracer_manager), [&context] {
        return std::make_shared<Tracing::HttpTracerManagerImpl>(
            std::make_unique<Tracing::TracerFactoryContextImpl>(
                context.getServerFactoryContext(), context.messageValidationVisitor()));
      });

  return {meta_route_config_provider_manager, http_tracer_manager};
}

Network::FilterFactoryCb MetaProtocolProxyFilterConfigFactory::createFilterFactoryFromProtoTyped(
    const aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy& proto_config,
    Server::Configuration::FactoryContext& context) {
  Utility::Singletons singletons = Utility::createSingletons(context);
  std::shared_ptr<Config> filter_config(std::make_shared<ConfigImpl>(
      proto_config, context, *singletons.route_config_provider_manager_,
      *singletons.http_tracer_manager_));

  // This lambda captures the shared_ptrs created above, thus preserving the
  // reference count.
  // Keep in mind the lambda capture list **doesn't** determine the destruction order, but it's fine
  // as these captured objects are also global singletons.
  return [singletons, filter_config, &context](Network::FilterManager& filter_manager) -> void {
    filter_manager.addReadFilter(std::make_shared<ConnectionManager>(
        *filter_config, context.api().randomGenerator(), context.dispatcher().timeSource()));
  };
}

/**
 * Static registration for the meta protocol filter. @see RegisterFactory.
 */
REGISTER_FACTORY(MetaProtocolProxyFilterConfigFactory,
                 Server::Configuration::NamedNetworkFilterConfigFactory);

// class ConfigImpl.
ConfigImpl::ConfigImpl(const MetaProtocolProxyConfig& config,
                       Server::Configuration::FactoryContext& context,
                       Route::RouteConfigProviderManager& route_config_provider_manager,
                       Tracing::HttpTracerManager& tracer_manager)
    : context_(context),
      stats_prefix_(
          fmt::format("meta_protocol.{}.{}.", config.application_protocol(), config.stat_prefix())),
      stats_(MetaProtocolProxyStats::generateStats(stats_prefix_, context_.scope())),
      application_protocol_(config.application_protocol()), codecConfig_(config.codec()) {
  switch (config.route_specifier_case()) {
  case aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy::RouteSpecifierCase::kRds:
    route_config_provider_ = route_config_provider_manager.createRdsRouteConfigProvider(
        config.rds(), context_.getServerFactoryContext(), stats_prefix_, context_.initManager());
    break;
  case aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy::RouteSpecifierCase::kRouteConfig:
    route_config_provider_ = route_config_provider_manager.createStaticRouteConfigProvider(
        config.route_config(), context_.getServerFactoryContext(),
        context_.messageValidationVisitor());
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  // route_matcher_ = std::make_unique<Router::RouteMatcherImpl>(config.route_config(), context);
  if (config.meta_protocol_filters().empty()) {
    ENVOY_LOG(debug, "using default router filter");

    aeraki::meta_protocol_proxy::v1alpha::MetaProtocolFilter router_config;
    router_config.set_name("aeraki.meta_protocol.filters.router");
    registerFilter(router_config);
  } else {
    for (const auto& filter_config : config.meta_protocol_filters()) {
      registerFilter(filter_config);
    }
  }

  if (config.has_tracing()) {
    initialTracer(config, context, tracer_manager);
  }
}
void ConfigImpl::initialTracer(const ConfigImpl::MetaProtocolProxyConfig& config,
                               const Server::Configuration::FactoryContext& context,
                               Tracing::HttpTracerManager& tracer_manager) {
  tracer_ = tracer_manager.getOrCreateHttpTracer(getPerFilterTracerConfig(config));

  const auto& tracing_config = config.tracing();

  Tracing::OperationName tracing_operation_name;

  // Listener level traffic direction overrides the operation name
  switch (context.direction()) {
  case envoy::config::core::v3::UNSPECIFIED: {
    switch (tracing_config.hidden_envoy_deprecated_operation_name()) {
    case envoy::extensions::filters::network::http_connection_manager::v3::
        HttpConnectionManager_Tracing::INGRESS:
      tracing_operation_name = Tracing::OperationName::Ingress;
      break;
    case envoy::extensions::filters::network::http_connection_manager::v3::
        HttpConnectionManager_Tracing::EGRESS:
      tracing_operation_name = Tracing::OperationName::Egress;
      break;
    default:
      NOT_REACHED_GCOVR_EXCL_LINE;
    }
    break;
  }
  case envoy::config::core::v3::INBOUND:
    tracing_operation_name = Tracing::OperationName::Ingress;
    break;
  case envoy::config::core::v3::OUTBOUND:
    tracing_operation_name = Tracing::OperationName::Egress;
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  Tracing::CustomTagMap custom_tags;
  for (const std::string& header :
       tracing_config.hidden_envoy_deprecated_request_headers_for_tags()) {
    envoy::type::tracing::v3::CustomTag::Header headerTag;
    headerTag.set_name(header);
    custom_tags.emplace(
        header, std::__1::make_shared<const Tracing::RequestHeaderCustomTag>(header, headerTag));
  }
  for (const auto& tag : tracing_config.custom_tags()) {
    custom_tags.emplace(tag.tag(), Tracing::HttpTracerUtility::createCustomTag(tag));
  }

  envoy::type::v3::FractionalPercent client_sampling;
  client_sampling.set_numerator(
      tracing_config.has_client_sampling() ? tracing_config.client_sampling().value() : 100);
  envoy::type::v3::FractionalPercent random_sampling;
  // TODO: Random sampling historically was an integer and default to out of 10,000. We should
  // deprecate that and move to a straight fractional percent config.
  uint64_t random_sampling_numerator{PROTOBUF_PERCENT_TO_ROUNDED_INTEGER_OR_DEFAULT(
      tracing_config, random_sampling, 10000, 10000)};
  random_sampling.set_numerator(random_sampling_numerator);
  random_sampling.set_denominator(envoy::type::v3::FractionalPercent::TEN_THOUSAND);
  envoy::type::v3::FractionalPercent overall_sampling;
  overall_sampling.set_numerator(
      tracing_config.has_overall_sampling() ? tracing_config.overall_sampling().value() : 100);

  const uint32_t max_path_tag_length = PROTOBUF_GET_WRAPPED_OR_DEFAULT(
      tracing_config, max_path_tag_length, Tracing::DefaultMaxPathTagLength);

  tracing_config_ =
      std::__1::make_unique<TracingConnectionManagerConfig>(TracingConnectionManagerConfig{
          tracing_operation_name, custom_tags, client_sampling, random_sampling, overall_sampling,
          tracing_config.verbose(), max_path_tag_length});
}

void ConfigImpl::createFilterChain(FilterChainFactoryCallbacks& callbacks) {
  for (const FilterFactoryCb& factory : filter_factories_) {
    factory(callbacks);
  }
}

Route::RouteConstSharedPtr ConfigImpl::route(const Metadata& metadata,
                                             uint64_t random_value) const {
  auto route_config = route_config_provider_->config();
  if (route_config) {
    return route_config->route(metadata, random_value);
  }
  ENVOY_LOG(error, "Failed to get Route Config");
  return nullptr;
  // return route_matcher_->route(metadata, random_value);
}

CodecPtr ConfigImpl::createCodec() {
  auto& factory = Envoy::Config::Utility::getAndCheckFactoryByName<NamedCodecConfigFactory>(
      codecConfig_.name());
  ProtobufTypes::MessagePtr message = factory.createEmptyConfigProto();
  Envoy::Config::Utility::translateOpaqueConfig(codecConfig_.config(),
                                                ProtobufWkt::Struct::default_instance(),
                                                context_.messageValidationVisitor(), *message);
  return factory.createCodec(*message);
}

void ConfigImpl::registerFilter(const MetaProtocolFilterConfig& proto_config) {
  const auto& string_name = proto_config.name();
  ENVOY_LOG(debug, "    meta protocol filter #{}", filter_factories_.size());
  ENVOY_LOG(debug, "      name: {}", string_name);
  ENVOY_LOG(debug, "    config: {}",
            MessageUtil::getJsonStringFromMessageOrError(proto_config.config(), true));

  auto& factory =
      Envoy::Config::Utility::getAndCheckFactoryByName<NamedMetaProtocolFilterConfigFactory>(
          string_name);
  ProtobufTypes::MessagePtr message = factory.createEmptyConfigProto();
  Envoy::Config::Utility::translateOpaqueConfig(proto_config.config(),
                                                ProtobufWkt::Struct::default_instance(),
                                                context_.messageValidationVisitor(), *message);
  FilterFactoryCb callback =
      factory.createFilterFactoryFromProto(*message, stats_prefix_, context_);

  filter_factories_.push_back(callback);
}

/**
 * Determines what tracing provider to use for a given
 * "envoy.filters.network.http_connection_manager" filter instance.
 */
const envoy::config::trace::v3::Tracing_Http*
ConfigImpl::getPerFilterTracerConfig(const MetaProtocolProxyConfig& config) {
  // Give precedence to tracing provider configuration defined as part of
  // "envoy.filters.network.http_connection_manager" filter config.
  if (config.tracing().has_provider()) {
    return &config.tracing().provider();
  }
  // Otherwise, for the sake of backwards compatibility, fallback to using tracing provider
  // configuration defined in the bootstrap config.
  if (context_.httpContext().defaultTracingConfig().has_http()) {
    return &context_.httpContext().defaultTracingConfig().http();
  }
  return nullptr;
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
