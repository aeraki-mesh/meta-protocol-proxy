#include "src/meta_protocol_proxy/filters/stats/stats.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "source/common/buffer/buffer_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Stats {
StatsFilter::StatsFilter(const aeraki::meta_protocol_proxy::filters::stats::v1alpha::Stats&,
                         const Server::Configuration::FactoryContext& context) {
  traffic_direction_ = context.direction();
}

FilterStatus StatsFilter::onMessageDecoded(MetadataSharedPtr, MutationSharedPtr mutation) {
  return FilterStatus::ContinueIteration;
}

FilterStatus StatsFilter::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::ContinueIteration;
}

} // namespace Stats
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
