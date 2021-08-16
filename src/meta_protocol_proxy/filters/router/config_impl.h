#pragma once

/*
#include <chrono>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
 */
#include <memory>
//#include <regex>
#include <string>
//#include <vector>

//#include "envoy/config/core/v3/base.pb.h"
//#include "envoy/config/route/v3/route_components.pb.h"
//#include "envoy/runtime/runtime.h"
//#include "envoy/server/filter_config.h"
//#include "envoy/type/v3/percent.pb.h"
//#include "envoy/upstream/cluster_manager.h"

//#include "source/common/common/matchers.h"
//#include "source/common/config/metadata.h"
//#include "source/common/http/hash_policy.h"
//#include "source/common/http/header_utility.h"
//#include "source/common/stats/symbol_table_impl.h"

//#include "absl/container/node_hash_map.h"
//#include "absl/types/optional.h"

#include "api/v1alpha/route.pb.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/filters/router/route.h"

/*
#include "src/meta_protocol_proxy/filters/router/rds/config_utility.h"
#include "src/meta_protocol_proxy/filters/router/rds/header_formatter.h"
#include "src/meta_protocol_proxy/filters/router/rds/header_parser.h"
#include "src/meta_protocol_proxy/filters/router/rds/metadatamatchcriteria_impl.h"
#include "src/meta_protocol_proxy/filters/router/rds/router_ratelimit.h"
#include "src/meta_protocol_proxy/filters/router/rds/tls_context_match_criteria_impl.h"
 */

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

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
  RouteConstSharedPtr route(const Metadata& metadata, uint64_t random_value) const override {
    return nullptr;
  }

private:
  const std::string name_;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
