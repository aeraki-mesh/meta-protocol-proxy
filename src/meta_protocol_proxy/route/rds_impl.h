#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <string>

#include "envoy/admin/v3/config_dump.pb.h"
#include "envoy/config/core/v3/config_source.pb.h"
#include "envoy/config/route/v3/route.pb.h"
#include "envoy/config/route/v3/route.pb.validate.h"
#include "envoy/config/subscription.h"
#include "envoy/http/codes.h"
#include "envoy/local_info/local_info.h"
#include "envoy/server/admin.h"
#include "envoy/server/filter_config.h"
#include "envoy/service/discovery/v3/discovery.pb.h"
#include "envoy/singleton/instance.h"
#include "envoy/stats/scope.h"
#include "envoy/thread_local/thread_local.h"

#include "source/common/common/callback_impl.h"
#include "source/common/common/cleanup.h"
#include "source/common/common/logger.h"
#include "source/common/config/subscription_base.h"
#include "source/common/init/manager_impl.h"
#include "source/common/init/target_impl.h"
#include "source/common/init/watcher_impl.h"
#include "source/common/protobuf/utility.h"

#include "absl/container/node_hash_map.h"
#include "absl/container/node_hash_set.h"
#include "absl/status/status.h"

#include "api/meta_protocol_proxy/admin/v1alpha/config_dump.pb.h"
#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.h"
#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.validate.h"
#include "api/meta_protocol_proxy/v1alpha/meta_protocol_proxy.pb.h"

#include "src/meta_protocol_proxy/route/rds.h"
#include "src/meta_protocol_proxy/route/route_config_provider_manager.h"
#include "src/meta_protocol_proxy/route/route_config_update_receiver.h"
#include "src/meta_protocol_proxy/route/route_config_update_receiver_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

class RouteConfigProviderManagerImpl;

/**
 * Implementation of RouteConfigProvider that holds a static route configuration.
 */
class StaticRouteConfigProviderImpl : public RouteConfigProvider {
public:
  StaticRouteConfigProviderImpl(
      const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration& config,
      Server::Configuration::ServerFactoryContext& factory_context,
      ProtobufMessage::ValidationVisitor& validator,
      RouteConfigProviderManagerImpl& route_config_provider_manager);
  ~StaticRouteConfigProviderImpl() override;

  // RouteConfigProvider
  ConfigConstSharedPtr config() override { return config_; }
  absl::optional<ConfigInfo> configInfo() const override {
    return ConfigInfo{route_config_proto_, ""};
  }
  SystemTime lastUpdated() const override { return last_updated_; }
  void onConfigUpdate() override {}
  void
  validateConfig(const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration&)
      const override {}

private:
  ConfigConstSharedPtr config_;
  aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration route_config_proto_;
  SystemTime last_updated_;
  RouteConfigProviderManagerImpl& route_config_provider_manager_;
};

/**
 * All RDS stats. @see stats_macros.h
 */
#define ALL_RDS_STATS(COUNTER, GAUGE)                                                              \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_empty)                                                                            \
  GAUGE(config_reload_time_ms, NeverImport)

/**
 * Struct definition for all RDS stats. @see stats_macros.h
 */
struct RdsStats {
  ALL_RDS_STATS(GENERATE_COUNTER_STRUCT, GENERATE_GAUGE_STRUCT)
};

class RdsRouteConfigProviderImpl;

/**
 * A class that fetches the route configuration dynamically using the RDS API and updates them to
 * RDS config providers.
 */
class RdsRouteConfigSubscription
    // The real configuration type is
    // aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration HTTP
    // RouteConfiguration is used here because we want to reuse the http rds grpc service
    : Envoy::Config::SubscriptionBase<envoy::config::route::v3::RouteConfiguration>,
      Logger::Loggable<Logger::Id::router> {
public:
  ~RdsRouteConfigSubscription() override;

  absl::optional<RouteConfigProvider*>& routeConfigProvider() { return route_config_provider_opt_; }
  RouteConfigUpdatePtr& routeConfigUpdate() { return config_update_info_; }

private:
  // Config::SubscriptionCallbacks
  absl::Status onConfigUpdate(const std::vector<Envoy::Config::DecodedResourceRef>& resources,
                              const std::string& version_info) override;
  absl::Status onConfigUpdate(const std::vector<Envoy::Config::DecodedResourceRef>& added_resources,
                              const Protobuf::RepeatedPtrField<std::string>& removed_resources,
                              const std::string& system_version_info) override;
  void onConfigUpdateFailed(Envoy::Config::ConfigUpdateFailureReason reason,
                            const EnvoyException* e) override;
  void httpRouteConfig2MetaProtocolRouteConfig(
      const envoy::config::route::v3::RouteConfiguration& http_route_config,
      aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration&
          meta_protocol_route_config);

  RdsRouteConfigSubscription(const aeraki::meta_protocol_proxy::v1alpha::Rds& rds,
                             const uint64_t manager_identifier,
                             Server::Configuration::ServerFactoryContext& factory_context,
                             const std::string& stat_prefix,
                             RouteConfigProviderManagerImpl& route_config_provider_manager);

  bool validateUpdateSize(int num_resources);

  const std::string route_config_name_;
  // This scope must outlive the subscription_ below as the subscription has derived stats.
  Stats::ScopeSharedPtr scope_;
  Envoy::Config::SubscriptionPtr subscription_;
  Server::Configuration::ServerFactoryContext& factory_context_;

  // Init target used to notify the parent init manager that the subscription [and its sub resource]
  // is ready.
  Init::SharedTargetImpl parent_init_target_;
  // Init watcher on RDS and VHDS ready event. This watcher marks parent_init_target_ ready.
  Init::WatcherImpl local_init_watcher_;
  // Target which starts the RDS subscription.
  Init::TargetImpl local_init_target_;
  Init::ManagerImpl local_init_manager_;
  std::string stat_prefix_;
  RdsStats stats_;
  RouteConfigProviderManagerImpl& route_config_provider_manager_;
  const uint64_t manager_identifier_;
  absl::optional<RouteConfigProvider*> route_config_provider_opt_;
  RouteConfigUpdatePtr config_update_info_;

  friend class RouteConfigProviderManagerImpl;
};

using RdsRouteConfigSubscriptionSharedPtr = std::shared_ptr<RdsRouteConfigSubscription>;

struct UpdateOnDemandCallback {
  const std::string alias_;
  Event::Dispatcher& thread_local_dispatcher_;
  std::weak_ptr<Http::RouteConfigUpdatedCallback> cb_;
};

/**
 * Implementation of RouteConfigProvider that fetches the route configuration dynamically using
 * the subscription.
 */
class RdsRouteConfigProviderImpl : public RouteConfigProvider,
                                   Logger::Loggable<Logger::Id::router> {
public:
  ~RdsRouteConfigProviderImpl() override;

  RdsRouteConfigSubscription& subscription() { return *subscription_; }

  // RouteConfigProvider
  ConfigConstSharedPtr config() override;
  absl::optional<ConfigInfo> configInfo() const override {
    return config_update_info_->configInfo();
  }
  SystemTime lastUpdated() const override { return config_update_info_->lastUpdated(); }
  void onConfigUpdate() override;
  void validateConfig(const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration&
                          config) const override;

private:
  struct ThreadLocalConfig : public ThreadLocal::ThreadLocalObject {
    ThreadLocalConfig(ConfigConstSharedPtr initial_config) : config_(std::move(initial_config)) {}
    ConfigConstSharedPtr config_;
  };

  RdsRouteConfigProviderImpl(RdsRouteConfigSubscriptionSharedPtr&& subscription,
                             Server::Configuration::ServerFactoryContext& factory_context);

  RdsRouteConfigSubscriptionSharedPtr subscription_;
  RouteConfigUpdatePtr& config_update_info_;
  Server::Configuration::ServerFactoryContext& factory_context_;
  ThreadLocal::TypedSlot<ThreadLocalConfig> tls_;
  // A flag used to determine if this instance of RdsRouteConfigProviderImpl hasn't been
  // deallocated. Please also see a comment in requestVirtualHostsUpdate() method implementation.
  std::shared_ptr<bool> still_alive_{std::make_shared<bool>(true)};

  friend class RouteConfigProviderManagerImpl;
};

using RdsRouteConfigProviderImplSharedPtr = std::shared_ptr<RdsRouteConfigProviderImpl>;

class RouteConfigProviderManagerImpl : public RouteConfigProviderManager,
                                       public Singleton::Instance,
                                       Logger::Loggable<Logger::Id::router> {
public:
  RouteConfigProviderManagerImpl(OptRef<Server::Admin> admin);

  std::unique_ptr<aeraki::meta_protocol_proxy::admin::v1alpha::RoutesConfigDump> dumpRouteConfigs(const Matchers::StringMatcher& name_matcher) const;

  // RouteConfigProviderManager
  RouteConfigProviderSharedPtr
  createRdsRouteConfigProvider(const aeraki::meta_protocol_proxy::v1alpha::Rds& rds,
                               Server::Configuration::ServerFactoryContext& factory_context,
                               const std::string& stat_prefix,
                               Init::Manager& init_manager) override;

  RouteConfigProviderPtr createStaticRouteConfigProvider(
      const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration& route_config,
      Server::Configuration::ServerFactoryContext& factory_context,
      ProtobufMessage::ValidationVisitor& validator) override;

private:
  // TODO(jsedgwick) These two members are prime candidates for the owned-entry list/map
  // as in ConfigTracker. I.e. the ProviderImpls would have an EntryOwner for these lists
  // Then the lifetime management stuff is centralized and opaque.
  absl::node_hash_map<uint64_t, std::weak_ptr<RdsRouteConfigProviderImpl>>
      dynamic_route_config_providers_;
  absl::node_hash_set<RouteConfigProvider*> static_route_config_providers_;
  Server::ConfigTracker::EntryOwnerPtr config_tracker_entry_;

  friend class RdsRouteConfigSubscription;
  friend class StaticRouteConfigProviderImpl;
};

using RouteConfigProviderManagerImplPtr = std::unique_ptr<RouteConfigProviderManagerImpl>;

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy