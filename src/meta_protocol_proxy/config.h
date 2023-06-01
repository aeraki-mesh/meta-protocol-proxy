#pragma once

#include <map>
#include <string>

#include "api/meta_protocol_proxy/v1alpha/meta_protocol_proxy.pb.h"
#include "api/meta_protocol_proxy/v1alpha/meta_protocol_proxy.pb.validate.h"

#include "envoy/access_log/access_log.h"
#include "envoy/tracing/trace_driver.h"

#include "source/extensions/filters/network/common/factory_base.h"
#include "source/extensions/filters/network/well_known_names.h"

#include "src/meta_protocol_proxy/conn_manager.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/route/route_config_provider_manager.h"
#include "src/meta_protocol_proxy/tracing/tracer_manager.h"
#include "src/meta_protocol_proxy/tracing/tracer.h"
#include "src/meta_protocol_proxy/tracing/tracer_impl.h"

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
    MetaProtocolProxy::Tracing::MetaProtocolTracerManagerSharedPtr tracer_manager_;
  };

  /**
   * Create/get singletons needed for config creation.
   *
   * @param context supplies the context used to create the singletons.
   * @return Singletons struct containing all the singletons.
   */
  static Singletons createSingletons(Server::Configuration::FactoryContext& context);
};

class ConfigImpl : public Config,
                   public Route::Config,
                   public FilterChainFactory,
                   Logger::Loggable<Logger::Id::config> {
public:
  using MetaProtocolProxyConfig = aeraki::meta_protocol_proxy::v1alpha::MetaProtocolProxy;
  using MetaProtocolFilterConfig = aeraki::meta_protocol_proxy::v1alpha::MetaProtocolFilter;
  using CodecConfig = aeraki::meta_protocol_proxy::v1alpha::Codec;
  using ApplicationProtocolConfig = aeraki::meta_protocol_proxy::v1alpha::ApplicationProtocol;

  ConfigImpl(const MetaProtocolProxyConfig& config, Server::Configuration::FactoryContext& context,
             Route::RouteConfigProviderManager& route_config_provider_manager,
             MetaProtocolProxy::Tracing::MetaProtocolTracerManager& tracer_manager);
  ~ConfigImpl() override {
    ENVOY_LOG(trace, "********** MetaProtocolProxy ConfigImpl destructed ***********");
  }

  // FilterChainFactory
  void createFilterChain(FilterChainFactoryCallbacks& callbacks) override;

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
  std::string applicationProtocol() override {
    return application_protocol_config_.name().empty() ? application_protocol_
                                                       : application_protocol_config_.name();
  };
  absl::optional<std::chrono::milliseconds> idleTimeout() override { return idle_timeout_; };
  Tracing::MetaProtocolTracerSharedPtr tracer() override { return tracer_; };
  Tracing::TracingConfig* tracingConfig() override { return tracing_config_.get(); };
  RequestIDExtensionSharedPtr requestIDExtension() override { return request_id_extension_; };
  const std::vector<AccessLog::InstanceSharedPtr>& accessLogs() const override {
    return access_logs_;
  }
  bool multiplexing() override { return application_protocol_config_.multiplexing(); }

private:
  void registerFilter(const MetaProtocolFilterConfig& proto_config);
  /**
   * Determines what tracing provider to use for a given
   * "envoy.filters.network.http_connection_manager" filter instance.
   */
  const envoy::config::trace::v3::Tracing_Http*
  getPerFilterTracerConfig(const MetaProtocolProxyConfig& config);

  const CodecConfig& getCodecConfig();

  Server::Configuration::FactoryContext& context_;
  // Router::RouteMatcherPtr route_matcher_;
  std::string application_protocol_;
  CodecConfig codecConfig_;
  ApplicationProtocolConfig application_protocol_config_;
  const std::string stats_prefix_;
  MetaProtocolProxyStats stats_;
  std::list<FilterFactoryCb> filter_factories_;
  Route::RouteConfigProviderSharedPtr route_config_provider_;
  Route::RouteConfigProviderManager& route_config_provider_manager_;
  absl::optional<std::chrono::milliseconds> idle_timeout_;
  MetaProtocolProxy::Tracing::MetaProtocolTracerSharedPtr tracer_{
      std::make_shared<MetaProtocolProxy::Tracing::NullTracer>()};
  Tracing::TracingConfigPtr tracing_config_;
  RequestIDExtensionSharedPtr request_id_extension_;
  std::vector<AccessLog::InstanceSharedPtr> access_logs_;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
