#pragma once

#include <string>

#include "api/meta_protocol_proxy/v1alpha/meta_protocol_proxy.pb.h"
#include "api/meta_protocol_proxy/v1alpha/meta_protocol_proxy.pb.validate.h"

#include "extensions/filters/network/common/factory_base.h"
#include "extensions/filters/network/well_known_names.h"
#include "envoy/tracing/http_tracer_manager.h"
#include "common/tracing/http_tracer_impl.h"

#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/route/route_matcher_impl.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/route/route_config_provider_manager.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

constexpr char CanonicalName[] = "aeraki.meta_protocol_proxy";

/**
 * Config registration for the meta protocol proxy filter. @see NamedNetworkFilterConfigFactory.
 */
class MetaProtocolProxyFilterConfigFactory
    : public Common::FactoryBase<aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy> {
public:
  MetaProtocolProxyFilterConfigFactory() : FactoryBase(CanonicalName, true) {}

private:
  Network::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy& proto_config,
      Server::Configuration::FactoryContext& context) override;
};

/**
 * Utility class for shared logic between meta protocol connection manager factories.
 */
class Utility {
public:
  struct Singletons {
    Route::RouteConfigProviderManagerSharedPtr route_config_provider_manager_;
    Tracing::HttpTracerManagerSharedPtr http_tracer_manager_;
  };

  /**
   * Create/get singletons needed for config creation.
   *
   * @param context supplies the context used to create the singletons.
   * @return Singletons struct containing all the singletons.
   */
  static Singletons createSingletons(Server::Configuration::FactoryContext& context);
};

/**
 * Configuration for tracing which is set on the connection manager level.
 * Http Tracing can be enabled/disabled on a per connection manager basis.
 * Here we specify some specific for connection manager settings.
 */
struct TracingConnectionManagerConfig {
  Tracing::OperationName operation_name_;
  Tracing::CustomTagMap custom_tags_;
  envoy::type::v3::FractionalPercent client_sampling_;
  envoy::type::v3::FractionalPercent random_sampling_;
  envoy::type::v3::FractionalPercent overall_sampling_;
  bool verbose_;
  uint32_t max_path_tag_length_;
};

using TracingConnectionManagerConfigPtr = std::unique_ptr<TracingConnectionManagerConfig>;

class ConfigImpl : public Config,
                   public Route::Config,
                   public FilterChainFactory,
                   Logger::Loggable<Logger::Id::config> {
public:
  using MetaProtocolProxyConfig = aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy;
  using MetaProtocolFilterConfig = aeraki::meta_protocol_proxy::v1alpha::MetaProtocolFilter;
  using CodecConfig = aeraki::meta_protocol_proxy::v1alpha::Codec;

  ConfigImpl(const MetaProtocolProxyConfig& config, Server::Configuration::FactoryContext& context,
             Route::RouteConfigProviderManager& route_config_provider_manager,
             Tracing::HttpTracerManager& http_tracer_manager);
  ~ConfigImpl() override = default;

  // FilterChainFactory
  void createFilterChain(FilterChainFactoryCallbacks& callbacks) override;

  Tracing::HttpTracerSharedPtr tracer() { return tracer_; }
  Route::RouteConfigProvider* routeConfigProvider() override {
    return route_config_provider_.get();
  }

  // Route::Config
  Route::RouteConstSharedPtr route(const Metadata& metadata, uint64_t random_value) const override;

  // Config
  MetaProtocolProxyStats& stats() override { return stats_; }
  FilterChainFactory& filterFactory() override { return *this; }
  Route::Config& routerConfig() override { return *this; }
  CodecPtr createCodec() override;
  std::string applicationProtocol() override { return application_protocol_; };
  const TracingConnectionManagerConfig* tracingConfig() { return tracing_config_.get(); }

private:
  void registerFilter(const MetaProtocolFilterConfig& proto_config);

  /**
   * Determines what tracing provider to use for a given
   * "envoy.filters.network.http_connection_manager" filter instance.
   */
  const envoy::config::trace::v3::Tracing_Http*
  getPerFilterTracerConfig(const MetaProtocolProxyConfig& filter_config);

  Server::Configuration::FactoryContext& context_;
  const std::string stats_prefix_;
  MetaProtocolProxyStats stats_;
  // Router::RouteMatcherPtr route_matcher_;
  std::string application_protocol_;
  CodecConfig codecConfig_;
  std::list<FilterFactoryCb> filter_factories_;
  Route::RouteConfigProviderSharedPtr route_config_provider_;
  TracingConnectionManagerConfigPtr tracing_config_;
  Tracing::HttpTracerSharedPtr tracer_{std::make_shared<Tracing::HttpNullTracer>()};
  void initialTracer(const MetaProtocolProxyConfig& config,
                     const Server::Configuration::FactoryContext& context,
                     Tracing::HttpTracerManager& tracer_manager);
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
