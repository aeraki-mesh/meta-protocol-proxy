#include "src/meta_protocol_proxy/filters/router/istio_stats.h"

#include <memory>
#include <string>
#include <vector>

#include "envoy/stats/scope.h"

#include "source/common/stats/symbol_table.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

IstioStats::IstioStats(Stats::Scope& scope)
    : scope_(scope), stat_name_set_(scope.symbolTable().makeSet("aerakicustom")),
      requests_total_(stat_name_set_->add("istio_requests_total")),
      request_duration_milliseconds_(stat_name_set_->add("istio_request_duration_milliseconds")),
      request_bytes_(stat_name_set_->add("istio_request_bytes")),
      response_bytes_(stat_name_set_->add("istio_response_bytes")), empty_(stat_name_set_->add("")),
      unknown_(stat_name_set_->add("unknown")), source_(stat_name_set_->add("source")),
      destination_(stat_name_set_->add("destination")), latest_(stat_name_set_->add("latest")),
      http_(stat_name_set_->add("http")), grpc_(stat_name_set_->add("grpc")),
      tcp_(stat_name_set_->add("tcp")), mutual_tls_(stat_name_set_->add("mutual_tls")),
      none_(stat_name_set_->add("none")), reporter_(stat_name_set_->add("reporter")),
      source_workload_(stat_name_set_->add("source_workload")),
      source_workload_namespace_(stat_name_set_->add("source_workload_namespace")),
      source_principal_(stat_name_set_->add("source_principal")),
      source_app_(stat_name_set_->add("source_app")),
      source_version_(stat_name_set_->add("source_version")),
      source_canonical_service_(stat_name_set_->add("source_canonical_service")),
      source_canonical_revision_(stat_name_set_->add("source_canonical_revision")),
      source_cluster_(stat_name_set_->add("source_cluster")),
      destination_workload_(stat_name_set_->add("destination_workload")),
      destination_workload_namespace_(stat_name_set_->add("destination_workload_namespace")),
      destination_principal_(stat_name_set_->add("destination_principal")),
      destination_app_(stat_name_set_->add("destination_app")),
      destination_version_(stat_name_set_->add("destination_version")),
      destination_service_(stat_name_set_->add("destination_service")),
      destination_service_name_(stat_name_set_->add("destination_service_name")),
      destination_service_namespace_(stat_name_set_->add("destination_service_namespace")),
      destination_canonical_service_(stat_name_set_->add("destination_canonical_service")),
      destination_canonical_revision_(stat_name_set_->add("destination_canonical_revision")),
      destination_cluster_(stat_name_set_->add("destination_cluster")),
      request_protocol_(stat_name_set_->add("request_protocol")),
      response_flags_(stat_name_set_->add("response_flags")),
      connection_security_policy_(stat_name_set_->add("connection_security_policy")),
      response_code_(stat_name_set_->add("response_code")) {}

void IstioStats::incCounter(const Stats::ElementVec& names) {
  Stats::Utility::counterFromElements(scope_, names).inc();
}

void IstioStats::recordHistogram(const Stats::ElementVec& names, Stats::Histogram::Unit unit,
                                 uint64_t sample) {
  Stats::Utility::histogramFromElements(scope_, names, unit).recordValue(sample);
}

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
