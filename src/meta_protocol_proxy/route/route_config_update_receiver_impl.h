#pragma once

#include <string>

#include "envoy/config/route/v3/route_components.pb.h"
#include "envoy/server/factory_context.h"
#include "envoy/service/discovery/v3/discovery.pb.h"

#include "source/common/common/logger.h"
#include "source/common/protobuf/utility.h"

#include "api/v1alpha/route.pb.h"

#include "src/meta_protocol_proxy/route/rds.h"
#include "src/meta_protocol_proxy/route/route_config_update_receiver.h"
#include "src/meta_protocol_proxy/route/config_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

class RouteConfigUpdateReceiverImpl : public RouteConfigUpdateReceiver {
public:
  RouteConfigUpdateReceiverImpl(Server::Configuration::ServerFactoryContext& factory_context)
      : factory_context_(factory_context), time_source_(factory_context.timeSource()),
        route_config_proto_(
            std::make_unique<envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::
                                 RouteConfiguration>()),
        last_config_hash_(0ull) {}

  bool onDemandFetchFailed(const envoy::service::discovery::v3::Resource& resource) const;
  void onUpdateCommon(const std::string& version_info);
  bool onRdsUpdate(
      const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
          rc,
      const std::string& version_info) override;
  const std::string& routeConfigName() const override { return route_config_proto_->name(); }
  const std::string& configVersion() const override { return last_config_version_; }
  uint64_t configHash() const override { return last_config_hash_; }
  absl::optional<RouteConfigProvider::ConfigInfo> configInfo() const override {
    return config_info_;
  }
  const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
  protobufConfiguration() override {
    return static_cast<const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::
                           RouteConfiguration&>(*route_config_proto_);
  }
  ConfigConstSharedPtr parsedConfiguration() const override { return config_; }
  SystemTime lastUpdated() const override { return last_updated_; }

private:
  Server::Configuration::ServerFactoryContext& factory_context_;
  TimeSource& time_source_;
  std::unique_ptr<
      envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration>
      route_config_proto_;
  uint64_t last_config_hash_;
  std::string last_config_version_;
  SystemTime last_updated_;
  absl::optional<RouteConfigProvider::ConfigInfo> config_info_;
  ConfigConstSharedPtr config_;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
