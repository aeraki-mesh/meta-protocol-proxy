#pragma once

#include <memory>

#include "api/meta_protocol_proxy/config/route/v1alpha/route.pb.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

/**
 * A provider for meta protocol route configurations.
 */
class RouteConfigProvider {
public:
  struct ConfigInfo {
    // A reference to the currently loaded route configuration. Do not hold this reference beyond
    // the caller of configInfo()'s scope.
    const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration& config_;

    // The discovery version that supplied this route. This will be set to "" in the case of
    // static routes.
    std::string version_;
  };

  virtual ~RouteConfigProvider() = default;

  /**
   * @return Route::ConfigConstSharedPtr a route configuration for use during a single request. The
   * returned config may be different on a subsequent call, so a new config should be acquired for
   * each request flow.
   */
  virtual ConfigConstSharedPtr config() PURE;

  /**
   * @return the configuration information for the currently loaded route configuration. Note that
   * if the provider has not yet performed an initial configuration load, no information will be
   * returned.
   */
  virtual absl::optional<ConfigInfo> configInfo() const PURE;

  /**
   * @return the last time this RouteConfigProvider was updated. Used for config dumps.
   */
  virtual SystemTime lastUpdated() const PURE;

  /**
   * Callback used to notify RouteConfigProvider about configuration changes.
   */
  virtual void onConfigUpdate() PURE;

  /**
   * Validate if the route configuration can be applied to the context of the route config provider.
   */
  virtual void validateConfig(
      const aeraki::meta_protocol_proxy::config::route::v1alpha::RouteConfiguration& config)
      const PURE;
};

using RouteConfigProviderPtr = std::unique_ptr<RouteConfigProvider>;
using RouteConfigProviderSharedPtr = std::shared_ptr<RouteConfigProvider>;

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
