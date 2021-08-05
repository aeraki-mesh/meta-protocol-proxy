#pragma once

#include <memory>
#include <string>

#include "envoy/config/typed_config.h"
#include "envoy/router/router.h"
#include "envoy/server/filter_config.h"

#include "api/v1alpha/route.pb.h"

#include "source/common/config/utility.h"
#include "source/common/singleton/const_singleton.h"
#include "src/meta_protocol_proxy/filters/router/router.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

using RouteConfigurations =
    envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration;

enum class RouteMatcherType : uint8_t {
  Default,
};

/**
 * Names of available Protocol implementations.
 */
class RouteMatcherNameValues {
public:
  struct RouteMatcherTypeHash {
    template <typename T> std::size_t operator()(T t) const { return static_cast<std::size_t>(t); }
  };

  using RouteMatcherNameMap =
      absl::node_hash_map<RouteMatcherType, std::string, RouteMatcherTypeHash>;

  const RouteMatcherNameMap routeMatcherNameMap = {
      {RouteMatcherType::Default, "default"},
  };

  const std::string& fromType(RouteMatcherType type) const {
    const auto& itor = routeMatcherNameMap.find(type);
    ASSERT(itor != routeMatcherNameMap.end());
    return itor->second;
  }
};

using RouteMatcherNames = ConstSingleton<RouteMatcherNameValues>;

class RouteMatcher {
public:
  virtual ~RouteMatcher() = default;

  virtual RouteConstSharedPtr route(const Metadata& metadata,
                                    uint64_t random_value) const PURE;
};

using RouteMatcherPtr = std::unique_ptr<RouteMatcher>;
using RouteMatcherConstSharedPtr = std::shared_ptr<const RouteMatcher>;

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
