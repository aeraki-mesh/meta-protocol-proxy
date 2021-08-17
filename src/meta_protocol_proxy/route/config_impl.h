#pragma once

#include <memory>
#include <string>

#include "envoy/server/factory_context.h"

#include "api/v1alpha/route.pb.h"

#include "src/meta_protocol_proxy/route/route.h"
#include "src/meta_protocol_proxy/route/route_matcher.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

/**
 * Implementation of Config that reads from a proto file.
 */
class ConfigImpl : public Config {
public:
  ConfigImpl(
      const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
          config,
      Server::Configuration::ServerFactoryContext& context);

  RouteConstSharedPtr route(const Metadata& metadata, uint64_t random_value) const override;

private:
  std::unique_ptr<RouteMatcher> route_matcher_;
  const std::string name_;
};

/**
 * Implementation of Config that is empty.
 */
class NullConfigImpl : public Config {
public:
  RouteConstSharedPtr route(const Metadata&, uint64_t) const override { return nullptr; }

private:
  const std::string name_;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
