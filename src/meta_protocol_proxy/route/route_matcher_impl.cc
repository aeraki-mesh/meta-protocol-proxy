#include "src/meta_protocol_proxy/route/route_matcher_impl.h"
#include "src/meta_protocol_proxy/codec_impl.h"
#include "envoy/config/route/v3/route_components.pb.h"
#include "api/v1alpha/route.pb.h"

#include "source/common/protobuf/utility.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

RouteEntryImplBase::RouteEntryImplBase(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Route& route)
    : cluster_name_(route.route().cluster()),
      config_headers_(Http::HeaderUtility::buildHeaderDataVector(route.match().metadata())) {
  if (route.route().cluster_specifier_case() ==
      envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteAction::
          ClusterSpecifierCase::kWeightedClusters) {
    total_cluster_weight_ = 0UL;
    for (const auto& cluster : route.route().weighted_clusters().clusters()) {
      weighted_clusters_.emplace_back(std::make_shared<WeightedClusterEntry>(*this, cluster));
      total_cluster_weight_ += weighted_clusters_.back()->clusterWeight();
    }
    ENVOY_LOG(debug, "meta protocol route matcher: weighted_clusters_size {}",
              weighted_clusters_.size());
  }
}

const std::string& RouteEntryImplBase::clusterName() const { return cluster_name_; }

const RouteEntry* RouteEntryImplBase::routeEntry() const { return this; }

RouteConstSharedPtr RouteEntryImplBase::clusterEntry(uint64_t random_value) const {
  if (weighted_clusters_.empty()) {
    ENVOY_LOG(debug, "meta protocol route matcher: weighted_clusters_size {}",
              weighted_clusters_.size());
    return shared_from_this();
  }

  return WeightedClusterUtil::pickCluster(weighted_clusters_, total_cluster_weight_, random_value,
                                          false);
}

bool RouteEntryImplBase::headersMatch(const Metadata& metadata) const {
  if (config_headers_.empty()) {
    ENVOY_LOG(debug, "meta protocol route matcher: no headers match");
    return true;
  }
  const MetadataImpl* metadataImpl = static_cast<const MetadataImpl*>(&metadata);
  const auto& headers = metadataImpl->getHeaders();
  ENVOY_LOG(debug, "meta protocol route matcher: headers size {}, metadata headers size {}",
            config_headers_.size(), headers.size());
  return Http::HeaderUtility::matchHeaders(headers, config_headers_);
}

RouteEntryImplBase::WeightedClusterEntry::WeightedClusterEntry(const RouteEntryImplBase& parent,
                                                               const WeightedCluster& cluster)
    : parent_(parent), cluster_name_(cluster.name()),
      cluster_weight_(PROTOBUF_GET_WRAPPED_REQUIRED(cluster, weight)) {}

RouteEntryImpl::RouteEntryImpl(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Route& route)
    : RouteEntryImplBase(route) {}

RouteEntryImpl::~RouteEntryImpl() = default;

RouteConstSharedPtr RouteEntryImpl::matches(const Metadata& metadata, uint64_t random_value) const {
  if (!RouteEntryImplBase::headersMatch(metadata)) {
    ENVOY_LOG(error, "meta protocol route matcher: headers not match");
    return nullptr;
  }

  return clusterEntry(random_value);
}

RouteMatcherImpl::RouteMatcherImpl(
    const RouteConfig& config,
    Server::Configuration::ServerFactoryContext&) { // TODO remove ServerFactoryContext parameter
  using envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteMatch;

  for (const auto& route : config.routes()) {
    routes_.emplace_back(std::make_shared<RouteEntryImpl>(route));
  }
  ENVOY_LOG(debug, "meta protocol route matcher: routes list size {}", routes_.size());
}

RouteConstSharedPtr RouteMatcherImpl::route(const Metadata& metadata, uint64_t random_value) const {
  for (const auto& route : routes_) {
    RouteConstSharedPtr route_entry = route->matches(metadata, random_value);
    if (nullptr != route_entry) {
      return route_entry;
    }
  }

  return nullptr;
}

} // namespace Route
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
