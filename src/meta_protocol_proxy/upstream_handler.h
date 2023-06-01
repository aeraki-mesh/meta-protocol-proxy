#pragma once

#include <memory>
#include <map>

#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"
#include "envoy/upstream/load_balancer.h"
#include "source/common/buffer/buffer_impl.h"
#include "src/meta_protocol_proxy/filters/filter_define.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

class UpstreamRequestCallbacks {
public:
  virtual ~UpstreamRequestCallbacks() = default;

  virtual void onPoolFailure(ConnectionPool::PoolFailureReason reason,
                             absl::string_view transport_failure_reason,
                             Upstream::HostDescriptionConstSharedPtr host) PURE;

  virtual void onPoolReady(Upstream::HostDescriptionConstSharedPtr host) PURE;
};

using ResponseCallback = std::function<void(MetadataSharedPtr response_metadata)>;
class UpstreamHandler {
public:
  virtual ~UpstreamHandler() = default;

  virtual int start(Upstream::TcpPoolData& pool_data) PURE;

  virtual void onData(Buffer::Instance& data, bool end_stream) PURE;

  virtual void addResponseCallback(uint64_t request_id, ResponseCallback callback) PURE;

  virtual bool isPoolReady() PURE;

  virtual void addUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) PURE;

  virtual void removeUpsteamRequestCallbacks(UpstreamRequestCallbacks* callbacks) PURE;

  static absl::optional<Upstream::TcpPoolData>
  createTcpPoolData(Upstream::ThreadLocalCluster& thread_local_cluster,
                    Upstream::LoadBalancerContext& context);
};
using UpstreamHandlerSharedPtr = std::shared_ptr<UpstreamHandler>;

class UpstreamHandlerManager : Logger::Loggable<Logger::Id::filter> {
public:
  void add(const std::string& key, std::shared_ptr<UpstreamHandler> client);
  void del(const std::string& key);
  std::shared_ptr<UpstreamHandler> get(const std::string& key);
  void clear();

private:
  // key: clusterName or clusterName_address
  std::map<std::string, std::shared_ptr<UpstreamHandler>> upstream_handlers_;
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
