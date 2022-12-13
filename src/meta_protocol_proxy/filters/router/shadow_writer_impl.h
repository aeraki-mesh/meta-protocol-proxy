#pragma once

#include <memory>

#include "envoy/buffer/buffer.h"
#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "source/common/common/linked_object.h"
#include "source/common/common/logger.h"
#include "source/common/buffer/buffer_impl.h"
#include "source/common/upstream/load_balancer_impl.h"

#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/filters/router/router.h"
#include "src/meta_protocol_proxy/filters/router/upstream_request.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

class NullResponseDecoder : public ResponseDecoderCallbacks,
                            public MessageHandler,
                            Logger::Loggable<Logger::Id::filter> {
public:
  NullResponseDecoder(CodecPtr codec)
      : codec_(std::move(codec)), decoder_(std::make_unique<ResponseDecoder>(*codec_, *this)) {}

  UpstreamResponseStatus decode(Buffer::Instance& data) {
    ENVOY_LOG(debug, "meta protocol shadow router: response: the received reply data length is {}",
              data.length());

    bool underflow = false;
    try {
      decoder_->onData(data, underflow);
    } catch (const EnvoyException& ex) {
      ENVOY_LOG(error, "meta protocol error: {}", ex.what());
      return UpstreamResponseStatus::Reset;
    }

    // decoder return underflow in th following two cases:
    // 1. decoder needs more data to complete the decoding of the current response, in this case,
    // the buffer contains part of the incomplete response.
    // 2. the response in the buffer have been processed and the buffer is already empty.
    //
    // Since underflow is also true when a response is completed, we need to use complete_ instead
    // of underflow to check whether the current response is completed or not.
    ASSERT(complete_ || underflow);
    return complete_ ? UpstreamResponseStatus::Complete : UpstreamResponseStatus::MoreData;
  }

  // StreamHandler
  void onMessageDecoded(MetadataSharedPtr metadata, MutationSharedPtr mutation) override {
    (void)metadata;
    (void)mutation;
    complete_ = true;
  };

  // ResponseDecoderCallbacks
  MessageHandler& newMessageHandler() override { return *this; };
  // Ignore the heartBeat from the upstream
  bool onHeartbeat(MetadataSharedPtr) override { return true; };

private:
  CodecPtr codec_;
  ResponseDecoderPtr decoder_;
  bool complete_ : 1;
};
using NullResponseDecoderPtr = std::unique_ptr<NullResponseDecoder>;

class ShadowWriterImpl;

class ShadowRouterImpl : public ShadowRouterHandle,
                         public RequestOwner,
                         public Tcp::ConnectionPool::UpstreamCallbacks,
                         public Upstream::LoadBalancerContextBase,
                         public Event::DeferredDeletable,
                         public LinkedObject<ShadowRouterImpl> {
public:
  ShadowRouterImpl(ShadowWriterImpl& parent, const std::string& cluster_name,
                   MetadataSharedPtr metadata, MutationSharedPtr mutation,
                   CodecFactory& codec_factory);
  ~ShadowRouterImpl() override {
    ENVOY_LOG(trace, "********** ShadowRouter destructed ***********");
  };

  bool createUpstreamRequest();
  // void maybeCleanup();
  void cleanup();

  // ShadowRouterHandle
  RequestOwner& requestOwner() override { return *this; }

  // RequestOwner
  Tcp::ConnectionPool::UpstreamCallbacks& upstreamCallbacks() override { return *this; }
  void continueDecoding() override{}; // todo
  void sendLocalReply(const DirectResponse& response, bool end_stream) override {
    (void)response;
    (void)end_stream;
  };
  CodecPtr createCodec() override { return std::move(codec_); };
  void resetStream() override { // TODO
    if (upstream_request_ != nullptr) {
      upstream_request_->releaseUpStreamConnection(true);
    }
  }
  void setUpstreamConnection(Tcp::ConnectionPool::ConnectionDataPtr conn) override { (void)conn; };
  void onUpstreamHostSelected(Upstream::HostDescriptionConstSharedPtr) override{};

  // Tcp::ConnectionPool::UpstreamCallbacks
  void onUpstreamData(Buffer::Instance& data, bool end_stream) override;
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

  // Upstream::LoadBalancerContextBase
  const Network::Connection* downstreamConnection() const override { return nullptr; }
  const Envoy::Router::MetadataMatchCriteria* metadataMatchCriteria() override { return nullptr; }

private:
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
  MutationSharedPtr mutation_;
  bool router_destroyed_{};
  Buffer::OwnedImpl upstream_request_buffer_;
  std::unique_ptr<UpstreamRequest> upstream_request_;
  uint64_t request_size_{};
  uint64_t response_size_{};
  bool request_ready_ : 1;

  std::list<ConverterCallback> pending_callbacks_;
  bool removed_{};

  CodecPtr codec_;
  NullResponseDecoder decoder_;
};

class ActiveRouters : public ThreadLocal::ThreadLocalObject,
                      public Logger::Loggable<Logger::Id::filter> {
public:
  ActiveRouters(Event::Dispatcher& dispatcher) : dispatcher_(dispatcher) {}
  // clean shadow router when dispatcher worker threads are destroyed
  ~ActiveRouters() override {
    ENVOY_LOG(trace, "********** ActiveRouters destructed ***********");
    while (!active_routers_.empty()) {
      auto& router = active_routers_.front();
      router->resetStream();
      remove(*router);
    }
  }

  std::list<std::unique_ptr<ShadowRouterImpl>>& activeRouters() { return active_routers_; }

  void remove(ShadowRouterImpl& router) {
    ENVOY_LOG(trace, "********** remove shadow router from active routers ***********");
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
    // Since ShadowWriter is shared across all the dispatcher worker threads, it isn't thread-safe
    // to just store shadow routers directly inside ShadowWriter.
    // We use a thread-local store for shadow writers. Each dispatcher worker thread holds an
    // ActiveRouters, which is a list of shadow routers for that thread.
    //
    //                                                  ---> Shadow routers list for worker thread 1
    // Router Config(Global) ---> ShadowWriter(Global)  ---> Shadow routers list for worker thread 2
    //                                                  ---> Shadow routers list for worker thread 3
    tls_->set([](Event::Dispatcher& dispatcher) -> ThreadLocal::ThreadLocalObjectSharedPtr {
      return std::make_shared<ActiveRouters>(dispatcher);
    });
  }

  ~ShadowWriterImpl() override {
    ENVOY_LOG(trace, "********** remove shadow writer from Router Config ***********");
  }

  void remove(ShadowRouterImpl& router) { tls_->getTyped<ActiveRouters>().remove(router); }

  // Router::ShadowWriter
  Upstream::ClusterManager& clusterManager() override { return cm_; }
  Event::Dispatcher& dispatcher() override { return dispatcher_; }
  void submit(const std::string& cluster_name, MetadataSharedPtr request_metadata,
              MutationSharedPtr mutation, CodecFactory& codec_factory) override;

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
