#include "src/meta_protocol_proxy/upstream_handler.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
absl::optional<Upstream::TcpPoolData>
UpstreamHandler::createTcpPoolData(Upstream::ThreadLocalCluster& thread_local_cluster,
                                   Upstream::LoadBalancerContext& context) {
  return thread_local_cluster.tcpConnPool(Upstream::ResourcePriority::Default, &context);
}

void UpstreamHandlerManager::add(const std::string& key, std::shared_ptr<UpstreamHandler> handler) {
  if (handler) {
    ENVOY_LOG(debug, "Add upstream handler, key:{}", key);
    upstream_handlers_.emplace(key, handler);
  }
}

void UpstreamHandlerManager::del(const std::string& key) {
  auto it = upstream_handlers_.find(key);
  if (it != upstream_handlers_.end()) {
    ENVOY_LOG(debug, "Del upstream handler key:{}", key);
    upstream_handlers_.erase(it);
  } else {
    ENVOY_LOG(debug, "Del upstream handler key:{} not find", key);
  }
}

std::shared_ptr<UpstreamHandler> UpstreamHandlerManager::get(const std::string& key) {
  auto it = upstream_handlers_.find(key);
  if (it != upstream_handlers_.end()) {
    return it->second;
  }
  return nullptr;
}

void UpstreamHandlerManager::clear() { upstream_handlers_.clear(); }

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
