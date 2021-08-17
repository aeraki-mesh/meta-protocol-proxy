#include "src/meta_protocol_proxy/route/route_config_update_receiver_impl.h"

#include <string>

#include "envoy/service/discovery/v3/discovery.pb.h"

#include "source/common/common/assert.h"
#include "source/common/common/fmt.h"
#include "source/common/common/thread.h"
#include "source/common/protobuf/utility.h"

#include "api/v1alpha/route.pb.h"

#include "src/meta_protocol_proxy/route/config_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Route {

bool RouteConfigUpdateReceiverImpl::onRdsUpdate(
    const envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration& rc,
    const std::string& version_info) {
  const uint64_t new_hash = MessageUtil::hash(rc);
  if (new_hash == last_config_hash_) {
    return false;
  }
  route_config_proto_ = std::make_unique<
      envoy::extensions::filters::network::meta_protocol_proxy::v1alpha::RouteConfiguration>(rc);
  last_config_hash_ = new_hash;
  config_ = std::make_shared<ConfigImpl>(*route_config_proto_, factory_context_);

  onUpdateCommon(version_info);
  return true;
}

void RouteConfigUpdateReceiverImpl::onUpdateCommon(const std::string& version_info) {
  last_config_version_ = version_info;
  last_updated_ = time_source_.systemTime();
  config_info_.emplace(RouteConfigProvider::ConfigInfo{*route_config_proto_, last_config_version_});
}

} // namespace Route
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
