#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/type/v3/range.pb.h"
#include "envoy/config/route/v3/route_components.pb.h"
#include "envoy/server/factory_context.h"

#include "api/v1alpha/route.pb.h"

#include "source/common/common/logger.h"
#include "source/common/common/matchers.h"
#include "source/common/http/header_utility.h"
#include "source/common/protobuf/protobuf.h"
#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/route/route_matcher.h"
#include "src/meta_protocol_proxy/route/route.h"

#include "absl/types/optional.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

class RouteEntryImplBase : public RouteEntry,
                           public Route,
                           public std::enable_shared_from_this<RouteEntryImplBase>,
                           public Logger::Loggable<Logger::Id::filter> {
public:
  RouteEntryImplBase(
      const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Route& route);
  ~RouteEntryImplBase() override = default;

  // Router::RouteEntry
  const std::string& clusterName() const override;
  const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() const override {
    return metadata_match_criteria_.get();
  }

  // Router::Route
  const RouteEntry* routeEntry() const override;

  virtual RouteConstSharedPtr matches(const Metadata& metadata, uint64_t random_value) const PURE;

protected:
  RouteConstSharedPtr clusterEntry(uint64_t random_value) const;
  bool headersMatch(const Metadata& metadata) const;

private:
  class WeightedClusterEntry : public RouteEntry, public Route {
  public:
    using WeightedCluster = envoy::config::route::v3::WeightedCluster::ClusterWeight;
    WeightedClusterEntry(const RouteEntryImplBase& parent, const WeightedCluster& cluster);

    uint64_t clusterWeight() const { return cluster_weight_; }

    // Router::RouteEntry
    const std::string& clusterName() const override { return cluster_name_; }
    const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() const override {
      return metadata_match_criteria_ ? metadata_match_criteria_.get()
                                      : parent_.metadataMatchCriteria();
    }

    // Router::Route
    const RouteEntry* routeEntry() const override { return this; }

  private:
    const RouteEntryImplBase& parent_;
    const std::string cluster_name_;
    const uint64_t cluster_weight_;
    Envoy::Router::MetadataMatchCriteriaConstPtr metadata_match_criteria_;
  };

  using WeightedClusterEntrySharedPtr = std::shared_ptr<WeightedClusterEntry>;

  uint64_t total_cluster_weight_;
  const std::string cluster_name_;
  const std::vector<Http::HeaderUtility::HeaderDataPtr> config_headers_;
  std::vector<WeightedClusterEntrySharedPtr> weighted_clusters_;

  // TODO(gengleilei) Implement it.
  Envoy::Router::MetadataMatchCriteriaConstPtr metadata_match_criteria_;
};

using RouteEntryImplBaseConstSharedPtr = std::shared_ptr<const RouteEntryImplBase>;

class RouteEntryImpl : public RouteEntryImplBase {
public:
  RouteEntryImpl(
      const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::Route& route);
  ~RouteEntryImpl() override;

  // RoutEntryImplBase
  RouteConstSharedPtr matches(const Metadata& metadata, uint64_t random_value) const override;
};

class RouteMatcherImpl : public RouteMatcher, public Logger::Loggable<Logger::Id::filter> {
public:
  using RouteConfig =
      envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration;

  RouteMatcherImpl(const RouteConfig& config, Server::Configuration::ServerFactoryContext& context);

  RouteConstSharedPtr route(const Metadata& metadata, uint64_t random_value) const override;

private:
  std::vector<RouteEntryImplBaseConstSharedPtr> routes_;
};

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
