#pragma once

#include <memory>

#include "envoy/buffer/buffer.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "source/common/common/linked_object.h"
#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/upstream/load_balancer_impl.h"

#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/filters/router/upstream_request.h"
#include "src/meta_protocol_proxy/route/route.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

class ShadowWriterImpl;

class ShadowRouterImpl : public ShadowRouterHandle,
                         public RequestOwner,
                         public Tcp::ConnectionPool::UpstreamCallbacks,
                         public Upstream::LoadBalancerContextBase,
                         public Event::DeferredDeletable,
                         public LinkedObject<ShadowRouterImpl> {
public:
  ShadowRouterImpl(ShadowWriterImpl& parent, const std::string& cluster_name,
                   MetadataSharedPtr& metadata);
  ~ShadowRouterImpl() override = default;

  bool createUpstreamRequest();
  void maybeCleanup();
  void resetStream() {
    if (upstream_request_ != nullptr) {
      upstream_request_->releaseUpStreamConnection(true);
    }
  }

  // ShadowRouterHandle
  void onRouterDestroy() override;
  bool waitingForConnection() const override;
  RequestOwner& requestOwner() override { return *this; }

  // RequestOwner
  Tcp::ConnectionPool::UpstreamCallbacks& upstreamCallbacks() override { return *this; }
  DecoderFilterCallbacks& decoderFilterCallbacks() override;

  // Tcp::ConnectionPool::UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override;
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // Upstream::LoadBalancerContextBase
  const Network::Connection* downstreamConnection() const override { return nullptr; }
  const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() override { return nullptr; }

  // Event::DeferredDeletable
  void deleteIsPending() override { deferred_deleting_ = true; }

private:
  friend class ShadowWriterTest;
  using ConverterCallback = std::function<FilterStatus()>;

  void writeRequest();
  bool requestInProgress();
  bool requestStarted() const;
  void flushPendingCallbacks();
  FilterStatus runOrSave(std::function<FilterStatus()>&& cb,
                         const std::function<void()>& on_save = {});

  ShadowWriterImpl& parent_;
  const std::string cluster_name_;
  MetadataSharedPtr metadata_;
  bool router_destroyed_{};
  bool request_sent_{};
  Buffer::OwnedImpl upstream_request_buffer_;
  std::unique_ptr<UpstreamRequest> upstream_request_;
  uint64_t request_size_{};
  uint64_t response_size_{};
  bool request_ready_ : 1;

  std::list<ConverterCallback> pending_callbacks_;
  bool removed_{};
  bool deferred_deleting_{};
};

class ActiveRouters : public ThreadLocal::ThreadLocalObject {
public:
  ActiveRouters(Event::Dispatcher& dispatcher) : dispatcher_(dispatcher) {}
  ~ActiveRouters() override {
    while (!active_routers_.empty()) {
      auto& router = active_routers_.front();
      router->resetStream();
      remove(*router);
    }
  }

  std::list<std::unique_ptr<ShadowRouterImpl>>& activeRouters() { return active_routers_; }

  void remove(ShadowRouterImpl& router) {
    dispatcher_.deferredDelete(router.removeFromList(active_routers_));
  }

private:
  Event::Dispatcher& dispatcher_;
  std::list<std::unique_ptr<ShadowRouterImpl>> active_routers_;
};

class ShadowWriterImpl : public ShadowWriter, Logger::Loggable<Logger::Id::filter> {
public:
  ShadowWriterImpl(Upstream::ClusterManager& cm, Event::Dispatcher& dispatcher,
                   ThreadLocal::SlotAllocator& tls)
      : cm_(cm), dispatcher_(dispatcher), tls_(tls.allocateSlot()) {
    tls_->set([](Event::Dispatcher& dispatcher) -> ThreadLocal::ThreadLocalObjectSharedPtr {
      return std::make_shared<ActiveRouters>(dispatcher);
    });
  }

  ~ShadowWriterImpl() override = default;

  void remove(ShadowRouterImpl& router) { tls_->getTyped<ActiveRouters>().remove(router); }

  // Router::ShadowWriter
  Upstream::ClusterManager& clusterManager() override { return cm_; }
  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  absl::optional<std::reference_wrapper<ShadowRouterHandle>>
  submit(const std::string& cluster_name, MetadataSharedPtr request_metadata) override;

private:
  friend class ShadowRouterImpl;

  Upstream::ClusterManager& cm_;
  Event::Dispatcher& dispatcher_;
  ThreadLocal::SlotPtr tls_;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
