#pragma once

#include <memory>

#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

class RouteMatcher {
public:
  virtual ~RouteMatcher() = default;

  virtual RouteConstSharedPtr route(const Metadata& metadata, uint64_t random_value) const PURE;
};

using RouteMatcherPtr = std::unique_ptr<RouteMatcher>;
using RouteMatcherConstSharedPtr = std::shared_ptr<const RouteMatcher>;

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
