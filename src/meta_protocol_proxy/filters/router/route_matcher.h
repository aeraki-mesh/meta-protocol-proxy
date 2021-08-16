#pragma once

#include <memory>

#include "src/meta_protocol_proxy/filters/router/router.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

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
