#pragma once

#include <memory>

#include "envoy/common/pure.h"
#include "envoy/common/time.h"
#include "api/v1alpha/route.pb.h"
#include "src/meta_protocol_proxy/route/rds.h"
#include "envoy/service/discovery/v3/discovery.pb.h"

#include "source/common/protobuf/protobuf.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

/**
 * A primitive that keeps track of updates to a RouteConfiguration.
 */
class RouteConfigUpdateReceiver {
public:
  virtual ~RouteConfigUpdateReceiver() = default;

  /**
   * Called on updates via RDS.
   * @param rc supplies the RouteConfiguration.
   * @param version_info supplies RouteConfiguration version.
   * @return bool whether RouteConfiguration has been updated.
   */
  virtual bool onRdsUpdate(
      const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
          rc,
      const std::string& version_info) PURE;

  using VirtualHostRefVector =
      std::vector<std::reference_wrapper<const envoy::config::route::v3::VirtualHost>>;

  /**
   * @return std::string& the name of RouteConfiguration.
   */
  virtual const std::string& routeConfigName() const PURE;

  /**
   * @return std::string& the version of RouteConfiguration.
   */
  virtual const std::string& configVersion() const PURE;

  /**
   * @return uint64_t the hash value of RouteConfiguration.
   */
  virtual uint64_t configHash() const PURE;

  /**
   * @return absl::optional<RouteConfigProvider::ConfigInfo> containing an instance of
   * RouteConfigProvider::ConfigInfo if RouteConfiguration has been updated at least once. Otherwise
   * returns an empty absl::optional<RouteConfigProvider::ConfigInfo>.
   */
  virtual absl::optional<RouteConfigProvider::ConfigInfo> configInfo() const PURE;

  /**
   * @return envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
   * current RouteConfiguration.
   */
  virtual const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::
      RouteConfiguration&
      protobufConfiguration() PURE;

  /**
   * @return Router::ConfigConstSharedPtr a parsed and validated copy of current RouteConfiguration.
   * @see protobufConfiguration()
   */
  virtual ConfigConstSharedPtr parsedConfiguration() const PURE;

  /**
   * @return SystemTime the time of the last update.
   */
  virtual SystemTime lastUpdated() const PURE;

  /**
   * @return the union of all resource names and aliases (if any) received with the last VHDS
   * update.
   */
  // virtual const std::set<std::string>& resourceIdsInLastVhdsUpdate() PURE;
};

using RouteConfigUpdatePtr = std::unique_ptr<RouteConfigUpdateReceiver>;

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
