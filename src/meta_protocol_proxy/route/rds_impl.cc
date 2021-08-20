#include "src/meta_protocol_proxy/route/rds_impl.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "envoy/admin/v3/config_dump.pb.h"
#include "envoy/config/core/v3/config_source.pb.h"
#include "envoy/service/discovery/v3/discovery.pb.h"

#include "source/common/common/assert.h"
#include "source/common/common/fmt.h"
#include "source/common/config/api_version.h"
#include "source/common/config/utility.h"
#include "source/common/config/version_converter.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/protobuf/utility.h"

#include "src/meta_protocol_proxy/route/config_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

StaticRouteConfigProviderImpl::StaticRouteConfigProviderImpl(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        config,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor&, // TODO Remove validator parameter
    RouteConfigProviderManagerImpl& route_config_provider_manager)
    : config_(new ConfigImpl(config, factory_context)), route_config_proto_{config},
      last_updated_(factory_context.timeSource().systemTime()),
      route_config_provider_manager_(route_config_provider_manager) {
  route_config_provider_manager_.static_route_config_providers_.insert(this);
}

StaticRouteConfigProviderImpl::~StaticRouteConfigProviderImpl() {
  route_config_provider_manager_.static_route_config_providers_.erase(this);
}

// TODO(htuch): If support for multiple clusters is added per #1170 cluster_name_
RdsRouteConfigSubscription::RdsRouteConfigSubscription(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Rds& rds,
    const uint64_t manager_identifier, Server::Configuration::ServerFactoryContext& factory_context,
    const std::string& stat_prefix, RouteConfigProviderManagerImpl& route_config_provider_manager)
    // The real configuration type is
    // Envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration HTTP
    // RouteConfiguration is used here because we want to reuse the http rds grpc service
    : Envoy::Config::SubscriptionBase<envoy::config::route::v3::RouteConfiguration>(
          rds.config_source().resource_api_version(),
          factory_context.messageValidationContext().dynamicValidationVisitor(), "name"),
      route_config_name_(rds.route_config_name()),
      scope_(factory_context.scope().createScope(stat_prefix + "rds." + route_config_name_ + ".")),
      factory_context_(factory_context),
      parent_init_target_(fmt::format("RdsRouteConfigSubscription init {}", route_config_name_),
                          [this]() { local_init_manager_.initialize(local_init_watcher_); }),
      local_init_watcher_(fmt::format("RDS local-init-watcher {}", rds.route_config_name()),
                          [this]() { parent_init_target_.ready(); }),
      local_init_target_(
          fmt::format("RdsRouteConfigSubscription local-init-target {}", route_config_name_),
          [this]() { subscription_->start({route_config_name_}); }),
      local_init_manager_(fmt::format("RDS local-init-manager {}", route_config_name_)),
      stat_prefix_(stat_prefix),
      stats_({ALL_RDS_STATS(POOL_COUNTER(*scope_), POOL_GAUGE(*scope_))}),
      route_config_provider_manager_(route_config_provider_manager),
      manager_identifier_(manager_identifier) {
  const auto resource_name = getResourceName();
  subscription_ =
      factory_context.clusterManager().subscriptionFactory().subscriptionFromConfigSource(
          rds.config_source(), Grpc::Common::typeUrl(resource_name), *scope_, *this,
          resource_decoder_, {});
  local_init_manager_.add(local_init_target_);
  config_update_info_ = std::make_unique<RouteConfigUpdateReceiverImpl>(factory_context);
}

RdsRouteConfigSubscription::~RdsRouteConfigSubscription() {
  // If we get destroyed during initialization, make sure we signal that we "initialized".
  local_init_target_.ready();

  // The ownership of RdsRouteConfigProviderImpl is shared among all HttpConnectionManagers that
  // hold a shared_ptr to it. The RouteConfigProviderManager holds weak_ptrs to the
  // RdsRouteConfigProviders. Therefore, the map entry for the RdsRouteConfigProvider has to get
  // cleaned by the RdsRouteConfigProvider's destructor.
  route_config_provider_manager_.dynamic_route_config_providers_.erase(manager_identifier_);
}

void RdsRouteConfigSubscription::onConfigUpdate(
    const std::vector<Envoy::Config::DecodedResourceRef>& resources,
    const std::string& version_info) {
  if (!validateUpdateSize(resources.size())) {
    return;
  }
  const auto& http_route_config = dynamic_cast<const envoy::config::route::v3::RouteConfiguration&>(
      resources[0].get().resource());

  auto meta_protocol_route_config =
      envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration();
  httpRouteConfig2MetaProtocolRouteConfig(http_route_config, meta_protocol_route_config);
  /*route_config.set_name(http_route_config.name());
  auto* route = route_config.add_routes();
  auto action =
      new envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteAction();
  action->set_cluster("outbound|20880||org.apache.dubbo.samples.basic.api.demoservice");
  route->set_allocated_route(action);*/

  if (meta_protocol_route_config.name() != route_config_name_) {
    throw EnvoyException(fmt::format("Unexpected RDS configuration (expecting {}): {}",
                                     route_config_name_, meta_protocol_route_config.name()));
  }
  if (route_config_provider_opt_.has_value()) {
    route_config_provider_opt_.value()->validateConfig(meta_protocol_route_config);
  }
  std::unique_ptr<Init::ManagerImpl> noop_init_manager;
  std::unique_ptr<Cleanup> resume_rds;
  if (config_update_info_->onRdsUpdate(meta_protocol_route_config, version_info)) {
    stats_.config_reload_.inc();
    stats_.config_reload_time_ms_.set(DateUtil::nowToMilliseconds(factory_context_.timeSource()));
    ENVOY_LOG(debug, "rds: loading new configuration: config_name={} hash={}", route_config_name_,
              config_update_info_->configHash());

    if (route_config_provider_opt_.has_value()) {
      route_config_provider_opt_.value()->onConfigUpdate();
    }
  }

  local_init_target_.ready();
}

// We use the Envoy RDS(HTTP RouteConfiguration) to transmit MetaProtocol RouteConfiguration between
// the RDS server and Envoy. The HTTP RouteConfiguration needs to be convert to MetaProtocol
// RouteConfiguration after received.
void RdsRouteConfigSubscription::httpRouteConfig2MetaProtocolRouteConfig(
    const envoy::config::route::v3::RouteConfiguration& http_route_config,
    envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        meta_protocol_route_config) {
  ASSERT(http_route_config.virtual_hosts_size() == 1);
  auto routeSize = http_route_config.virtual_hosts(0).routes_size();
  ASSERT(routeSize > 0);

  meta_protocol_route_config.set_name(http_route_config.name());

  for (int i = 0; i < routeSize; i++) {
    auto* metaRoute = meta_protocol_route_config.add_routes();
    auto httpRoute = http_route_config.virtual_hosts(0).routes(i);
    metaRoute->set_name(httpRoute.name());
    if (httpRoute.has_match()) {
      auto httpMatch = httpRoute.match();
      auto headerSize = httpMatch.headers_size();
      ASSERT(headerSize > 1);
      auto* metaMatch =
          new envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteMatch();

      for (int i = 0; i < headerSize; i++) {
        metaMatch->mutable_metadata()->AddAllocated(
            new envoy::config::route::v3::HeaderMatcher(httpMatch.headers(i)));
      }
      metaRoute->set_allocated_match(metaMatch);
    }

    ASSERT(httpRoute.has_route());

    auto* action =
        new envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteAction();
    if (httpRoute.route().cluster_specifier_case() ==
        envoy::config::route::v3::RouteAction::ClusterSpecifierCase::kCluster) {
      action->set_allocated_cluster(new std::string(httpRoute.route().cluster()));
    } else if (httpRoute.route().cluster_specifier_case() ==
               envoy::config::route::v3::RouteAction::ClusterSpecifierCase::kWeightedClusters) {
      action->set_allocated_weighted_clusters(
          new envoy::config::route::v3::WeightedCluster(httpRoute.route().weighted_clusters()));
    }
    metaRoute->set_allocated_route(action);
  }
}

void RdsRouteConfigSubscription::onConfigUpdate(
    const std::vector<Envoy::Config::DecodedResourceRef>& added_resources,
    const Protobuf::RepeatedPtrField<std::string>& removed_resources, const std::string&) {
  if (!removed_resources.empty()) {
    // TODO(#2500) when on-demand resource loading is supported, an RDS removal may make sense
    // (see discussion in #6879), and so we should do something other than ignoring here.
    ENVOY_LOG(
        error,
        "Server sent a delta RDS update attempting to remove a resource (name: {}). Ignoring.",
        removed_resources[0]);
  }
  if (!added_resources.empty()) {
    onConfigUpdate(added_resources, added_resources[0].get().version());
  }
}

void RdsRouteConfigSubscription::onConfigUpdateFailed(
    Envoy::Config::ConfigUpdateFailureReason reason, const EnvoyException*) {
  ASSERT(Envoy::Config::ConfigUpdateFailureReason::ConnectionFailure != reason);
  // We need to allow server startup to continue, even if we have a bad
  // config.
  local_init_target_.ready();
}

bool RdsRouteConfigSubscription::validateUpdateSize(int num_resources) {
  if (num_resources == 0) {
    ENVOY_LOG(debug, "Missing RouteConfiguration for {} in onConfigUpdate()", route_config_name_);
    stats_.update_empty_.inc();
    local_init_target_.ready();
    return false;
  }
  if (num_resources != 1) {
    throw EnvoyException(fmt::format("Unexpected RDS resource length: {}", num_resources));
    // (would be a return false here)
  }
  return true;
}

RdsRouteConfigProviderImpl::RdsRouteConfigProviderImpl(
    RdsRouteConfigSubscriptionSharedPtr&& subscription,
    Server::Configuration::ServerFactoryContext& factory_context)
    : subscription_(std::move(subscription)),
      config_update_info_(subscription_->routeConfigUpdate()), factory_context_(factory_context),
      tls_(factory_context.threadLocal()) {
  ConfigConstSharedPtr initial_config;
  if (config_update_info_->configInfo().has_value()) {
    initial_config = std::make_shared<ConfigImpl>(config_update_info_->protobufConfiguration(),
                                                  factory_context_);
  } else {
    initial_config = std::make_shared<NullConfigImpl>();
  }
  tls_.set([initial_config](Event::Dispatcher&) {
    return std::make_shared<ThreadLocalConfig>(initial_config);
  });
  // It should be 1:1 mapping due to shared rds config.
  ASSERT(!subscription_->routeConfigProvider().has_value());
  subscription_->routeConfigProvider().emplace(this);
}

RdsRouteConfigProviderImpl::~RdsRouteConfigProviderImpl() {
  ASSERT(subscription_->routeConfigProvider().has_value());
  subscription_->routeConfigProvider().reset();
}

ConfigConstSharedPtr RdsRouteConfigProviderImpl::config() { return tls_->config_; }

void RdsRouteConfigProviderImpl::onConfigUpdate() {
  tls_.runOnAllThreads([new_config = config_update_info_->parsedConfiguration()](
                           OptRef<ThreadLocalConfig> tls) { tls->config_ = new_config; });
}

void RdsRouteConfigProviderImpl::validateConfig(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        config) const {
  // TODO(lizan): consider cache the config here until onConfigUpdate.
  ConfigImpl validation_config(config, factory_context_);
}

RouteConfigProviderManagerImpl::RouteConfigProviderManagerImpl(Server::Admin& admin) {
  config_tracker_entry_ =
      admin.getConfigTracker().add("routes", [this](const Matchers::StringMatcher& matcher) {
        return dumpRouteConfigs(matcher);
      });
  // ConfigTracker keys must be unique. We are asserting that no one has stolen the "routes" key
  // from us, since the returned entry will be nullptr if the key already exists.
  RELEASE_ASSERT(config_tracker_entry_, "");
}

RouteConfigProviderSharedPtr RouteConfigProviderManagerImpl::createRdsRouteConfigProvider(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Rds& rds,
    Server::Configuration::ServerFactoryContext& factory_context, const std::string& stat_prefix,
    Init::Manager& init_manager) {
  // RdsRouteConfigSubscriptions are unique based on their serialized RDS config.
  const uint64_t manager_identifier = MessageUtil::hash(rds);
  auto it = dynamic_route_config_providers_.find(manager_identifier);

  if (it == dynamic_route_config_providers_.end()) {
    // std::make_shared does not work for classes with private constructors. There are ways
    // around it. However, since this is not a performance critical path we err on the side
    // of simplicity.

    RdsRouteConfigSubscriptionSharedPtr subscription(new RdsRouteConfigSubscription(
        rds, manager_identifier, factory_context, stat_prefix, *this));
    init_manager.add(subscription->parent_init_target_);
    RdsRouteConfigProviderImplSharedPtr new_provider{
        new RdsRouteConfigProviderImpl(std::move(subscription), factory_context)};
    dynamic_route_config_providers_.insert(
        {manager_identifier, std::weak_ptr<RdsRouteConfigProviderImpl>(new_provider)});
    return new_provider;
  } else {
    // Because the RouteConfigProviderManager's weak_ptrs only get cleaned up
    // in the RdsRouteConfigSubscription destructor, and the single threaded nature
    // of this code, locking the weak_ptr will not fail.
    auto existing_provider = it->second.lock();
    RELEASE_ASSERT(existing_provider != nullptr,
                   absl::StrCat("cannot find subscribed rds resource ", rds.route_config_name()));
    init_manager.add(existing_provider->subscription_->parent_init_target_);
    return existing_provider;
  }
}

RouteConfigProviderPtr RouteConfigProviderManagerImpl::createStaticRouteConfigProvider(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        route_config,
    Server::Configuration::ServerFactoryContext& factory_context,
    ProtobufMessage::ValidationVisitor& validator) {
  auto provider = std::make_unique<StaticRouteConfigProviderImpl>(
      route_config, factory_context, validator, *this); // TODO remove validator
  static_route_config_providers_.insert(provider.get());
  return provider;
}

std::unique_ptr<envoy::admin::v3::RoutesConfigDump>
RouteConfigProviderManagerImpl::dumpRouteConfigs(
    const Matchers::StringMatcher& name_matcher) const {
  auto config_dump = std::make_unique<envoy::admin::v3::RoutesConfigDump>();

  for (const auto& element : dynamic_route_config_providers_) {
    const auto& subscription = element.second.lock()->subscription_;
    // Because the RouteConfigProviderManager's weak_ptrs only get cleaned up
    // in the RdsRouteConfigSubscription destructor, and the single threaded nature
    // of this code, locking the weak_ptr will not fail.
    ASSERT(subscription);
    ASSERT(subscription->route_config_provider_opt_.has_value());

    if (subscription->routeConfigUpdate()->configInfo()) {
      if (!name_matcher.match(subscription->routeConfigUpdate()->protobufConfiguration().name())) {
        continue;
      }
      auto* dynamic_config = config_dump->mutable_dynamic_route_configs()->Add();
      dynamic_config->set_version_info(subscription->routeConfigUpdate()->configVersion());
      dynamic_config->mutable_route_config()->PackFrom(
          API_RECOVER_ORIGINAL(subscription->routeConfigUpdate()->protobufConfiguration()));
      TimestampUtil::systemClockToTimestamp(subscription->routeConfigUpdate()->lastUpdated(),
                                            *dynamic_config->mutable_last_updated());
    }
  }

  for (const auto& provider : static_route_config_providers_) {
    ASSERT(provider->configInfo());
    if (!name_matcher.match(provider->configInfo().value().config_.name())) {
      continue;
    }
    auto* static_config = config_dump->mutable_static_route_configs()->Add();
    static_config->mutable_route_config()->PackFrom(
        API_RECOVER_ORIGINAL(provider->configInfo().value().config_));
    TimestampUtil::systemClockToTimestamp(provider->lastUpdated(),
                                          *static_config->mutable_last_updated());
  }

  return config_dump;
}

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
