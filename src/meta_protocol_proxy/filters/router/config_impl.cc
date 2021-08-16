#include "src/meta_protocol_proxy/filters/router/config_impl.h"

#include <memory>
/*
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


#include "api/v1alpha/route.pb.h"
#include "envoy/config/route/v3/route_components.pb.h"
#include "envoy/http/header_map.h"
#include "envoy/runtime/runtime.h"
#include "envoy/type/matcher/v3/string.pb.h"
#include "envoy/type/v3/percent.pb.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/upstream/upstream.h"

#include "source/common/common/assert.h"
#include "source/common/common/empty_string.h"
#include "source/common/common/fmt.h"
#include "source/common/common/hash.h"
#include "source/common/common/logger.h"
#include "source/common/common/regex.h"
#include "source/common/common/utility.h"
#include "source/common/config/metadata.h"
#include "source/common/config/utility.h"
#include "source/common/config/well_known_names.h"
#include "source/common/http/headers.h"
#include "source/common/http/path_utility.h"
#include "source/common/protobuf/protobuf.h"
#include "source/common/protobuf/utility.h"
#include "src/meta_protocol_proxy/filters/router/rds/reset_header_parser.h"
#include "src/meta_protocol_proxy/filters/router/rds/retry_state_impl.h"
#include "source/common/runtime/runtime_features.h"
#include "source/common/tracing/http_tracer_impl.h"
#include "src/meta_protocol_proxy/filters/router/rds/http_utility.h"
#include "src/meta_protocol_proxy/filters/router/rds/filter_utility.h"
*/

#include "envoy/config/core/v3/base.pb.h"

#include "src/meta_protocol_proxy/filters/router/route.h"
#include "src/meta_protocol_proxy/filters/router/route_matcher_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

ConfigImpl::ConfigImpl(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration&
        config,
    Server::Configuration::ServerFactoryContext& context)
    : name_(config.name()) {
  route_matcher_ = std::make_unique<
      Envoy::Extensions::NetworkFilters::MetaProtocolProxy::Router::RouteMatcherImpl>(config,
                                                                                      context);
}

RouteConstSharedPtr ConfigImpl::route(const Metadata& metadata, uint64_t random_value) const {
  return route_matcher_->route(metadata, random_value);
}

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
