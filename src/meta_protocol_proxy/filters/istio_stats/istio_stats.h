#pragma once

#include <memory>
#include <string>
#include <vector>

#include "envoy/stats/scope.h"
#include "envoy/config/core/v3/base.pb.h"
#include "envoy/server/factory_context.h"

#include "source/common/stats/symbol_table.h"
#include "source/common/stats/utility.h"

// istio proxy
#include "extensions/common/proto_util.h"
#include "extensions/common/context.h"

#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace IstioStats {
constexpr absl::string_view CustomStatNamespace =
    "wasmcustom"; // use wasmcustom namespace to keep compatible with istio
class IstioStats {
public:
  IstioStats(Server::Configuration::FactoryContext& context,
             envoy::config::core::v3::TrafficDirection traffic_direction);

  void incCounter(const ::Wasm::Common::FlatNode& node, MetadataSharedPtr metadata);
  void recordHistogram(const Stats::ElementVec& names, Stats::Histogram::Unit unit,
                       uint64_t sample);

private:
  void populateSourceTags(const Wasm::Common::FlatNode& node, Stats::StatNameTagVector& tags);
  void populateDestinationTags(const Wasm::Common::FlatNode& node, Stats::StatNameTagVector& tags);
  // traffic direction, inbound or outbound
  envoy::config::core::v3::TrafficDirection traffic_direction_;
  flatbuffers::DetachedBuffer local_node_info_;
  Stats::Scope& scope_;
  Stats::StatNameDynamicPool pool_;
  absl::flat_hash_map<std::string, Stats::StatName> all_metrics_;
  absl::flat_hash_map<std::string, Stats::StatName> all_tags_;

public:
  // Metric names
  const Stats::StatName stat_namespace_;
  const Stats::StatName requests_total_;
  const Stats::StatName request_duration_milliseconds_;
  const Stats::StatName request_bytes_;
  const Stats::StatName response_bytes_;

  // Constant names.
  const Stats::StatName empty_;
  const Stats::StatName unknown_;
  const Stats::StatName source_;
  const Stats::StatName destination_;
  const Stats::StatName latest_;
  const Stats::StatName http_;
  const Stats::StatName grpc_;
  const Stats::StatName tcp_;
  const Stats::StatName mutual_tls_;
  const Stats::StatName none_;

  // Tag names
  const Stats::StatName reporter_;

  const Stats::StatName source_workload_;
  const Stats::StatName source_workload_namespace_;
  const Stats::StatName source_principal_;
  const Stats::StatName source_app_;
  const Stats::StatName source_version_;
  const Stats::StatName source_canonical_service_;
  const Stats::StatName source_canonical_revision_;
  const Stats::StatName source_cluster_;

  const Stats::StatName destination_workload_;
  const Stats::StatName destination_workload_namespace_;
  const Stats::StatName destination_principal_;
  const Stats::StatName destination_app_;
  const Stats::StatName destination_version_;
  const Stats::StatName destination_service_;
  const Stats::StatName destination_service_name_;
  const Stats::StatName destination_service_namespace_;
  const Stats::StatName destination_canonical_service_;
  const Stats::StatName destination_canonical_revision_;
  const Stats::StatName destination_cluster_;

  const Stats::StatName request_protocol_;
  const Stats::StatName response_flags_;
  const Stats::StatName connection_security_policy_;
  const Stats::StatName response_code_;

  // Per-process constants.
  const Stats::StatName workload_name_;
  const Stats::StatName namespace_;
  const Stats::StatName canonical_name_;
  const Stats::StatName canonical_revision_;
  const Stats::StatName cluster_name_;
  const Stats::StatName app_name_;
  const Stats::StatName app_version_;

  // istio_build metric:
  // Publishes Istio version for the proxy as a gauge, sample data:
  // testdata/metric/istio_build.yaml
  // Sample value for istio_version: "1.17.0"
  const Stats::StatName istio_build_;
  const Stats::StatName component_;
  const Stats::StatName proxy_;
  const Stats::StatName tag_;
  const Stats::StatName istio_version_;
};

} // namespace IstioStats
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
