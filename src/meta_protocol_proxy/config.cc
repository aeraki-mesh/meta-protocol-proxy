#include "src/meta_protocol_proxy/config.h"

#include "absl/container/flat_hash_map.h"

#include "envoy/registry/registry.h"
#include "source/common/config/utility.h"

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
SINGLETON_MANAGER_REGISTRATION(meta_route_config_provider_manager);

Utility::Singletons Utility::createSingletons(Server::Configuration::FactoryContext& context) {
  Route::RouteConfigProviderManagerSharedPtr meta_route_config_provider_manager =
      context.singletonManager().getTyped<Route::RouteConfigProviderManager>(
          SINGLETON_MANAGER_REGISTERED_NAME(meta_route_config_provider_manager), [&context] {
            return std::make_shared<Route::RouteConfigProviderManagerImpl>(context.admin());
          });
  return {meta_route_config_provider_manager};
}

Network::FilterFactoryCb MetaProtocolProxyFilterConfigFactory::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::MetaProtocolProxy&
        proto_config,
    Server::Configuration::FactoryContext& context) {
  Utility::Singletons singletons = Utility::createSingletons(context);
  std::shared_ptr<Config> filter_config(std::make_shared<ConfigImpl>(
      proto_config, context, *singletons.route_config_provider_manager_));

  return [filter_config, &context](Network::FilterManager& filter_manager) -> void {
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
                       Route::RouteConfigProviderManager& route_config_provider_manager)
    : context_(context),
      stats_prefix_(
          fmt::format("meta_protocol.{}.{}.", config.application_protocol(), config.stat_prefix())),
      stats_(MetaProtocolProxyStats::generateStats(stats_prefix_, context_.scope())),
      application_protocol_(config.application_protocol()), codecConfig_(config.codec()),
      route_config_provider_manager_(route_config_provider_manager) {
  switch (config.route_specifier_case()) {
  case envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::MetaProtocolProxy::
      RouteSpecifierCase::kRds:
    route_config_provider_ = route_config_provider_manager_.createRdsRouteConfigProvider(
        config.rds(), context_.getServerFactoryContext(), stats_prefix_, context_.initManager());
    break;
  case envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::MetaProtocolProxy::
      RouteSpecifierCase::kRouteConfig:
    route_config_provider_ = route_config_provider_manager_.createStaticRouteConfigProvider(
        config.route_config(), context_.getServerFactoryContext(),
        context_.messageValidationVisitor());
    break;
  default:
    NOT_REACHED_GCOVR_EXCL_LINE;
  }

  // route_matcher_ = std::make_unique<Router::RouteMatcherImpl>(config.route_config(), context);
  if (config.meta_protocol_filters().empty()) {
    ENVOY_LOG(debug, "using default router filter");

    envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::MetaProtocolFilter
        router_config;
    router_config.set_name("aeraki.meta_protocol.filters.router");
    registerFilter(router_config);
  } else {
    for (const auto& filter_config : config.meta_protocol_filters()) {
      registerFilter(filter_config);
    }
  }
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

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
