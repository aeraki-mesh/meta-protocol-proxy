#include "src/meta_protocol_proxy/config.h"

#include "absl/container/flat_hash_map.h"

#include "envoy/registry/registry.h"
#include "source/common/config/utility.h"

#include "src/meta_protocol_proxy/codec/factory.h"
#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/filters/factory_base.h"
#include "src/meta_protocol_proxy/filters/router/route_matcher.h"
#include "src/meta_protocol_proxy/stats.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

Network::FilterFactoryCb MetaProtocolProxyFilterConfigFactory::createFilterFactoryFromProtoTyped(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::MetaProtocolProxy&
        proto_config,
    Server::Configuration::FactoryContext& context) {
  std::shared_ptr<Config> filter_config(std::make_shared<ConfigImpl>(proto_config, context));

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
                       Server::Configuration::FactoryContext& context)
    : context_(context),
      stats_prefix_(
          fmt::format("meta_protocol.{}.{}.", config.application_protocol(), config.stat_prefix())),
      stats_(MetaProtocolProxyStats::generateStats(stats_prefix_, context_.scope())),
      application_protocol_(config.application_protocol()), codecConfig_(config.codec()) {
  route_matcher_ = std::make_unique<Router::RouteMatcherImpl>(config.route_config(), context);
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

Router::RouteConstSharedPtr ConfigImpl::route(const Metadata& metadata,
                                              uint64_t random_value) const {
  return route_matcher_->route(metadata, random_value);
}

CodecPtr ConfigImpl::createCodec() {
  auto& factory =
      Envoy::Config::Utility::getAndCheckFactoryByName<NamedCodecConfigFactory>(
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

  auto& factory = Envoy::Config::Utility::getAndCheckFactoryByName<
      NamedMetaProtocolFilterConfigFactory>(string_name);
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
