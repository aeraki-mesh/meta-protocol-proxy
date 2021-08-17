#pragma once

#include "envoy/common/time.h"
#include "api/v1alpha/meta_protocol_proxy.pb.h"
#include "envoy/network/connection.h"
#include "envoy/network/filter.h"
#include "envoy/stats/scope.h"
#include "envoy/stats/stats.h"
#include "envoy/stats/stats_macros.h"
#include "envoy/stats/timespan.h"

#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/active_message.h"
#include "src/meta_protocol_proxy/decoder.h"
#include "src/meta_protocol_proxy/decoder_event_handler.h"
#include "src/meta_protocol_proxy/filters/filter.h"
#include "src/meta_protocol_proxy/stats.h"
#include "src/meta_protocol_proxy/route/rds.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

/**
 * Config is a configuration interface for ConnectionManager.
 */
class Config {
public:
  virtual ~Config() = default;

  virtual FilterChainFactory& filterFactory() PURE;
  virtual MetaProtocolProxyStats& stats() PURE;
  virtual CodecPtr createCodec() PURE;
  virtual Route::Config& routerConfig() PURE;
  virtual std::string applicationProtocol() PURE;
  /**
   * @return Route::RouteConfigProvider* the configuration provider used to acquire a route
   *         config for each request flow. Pointer ownership is _not_ transferred to the caller of
   *         this function.
   */
  virtual Route::RouteConfigProvider* routeConfigProvider() PURE;
};

// class ActiveMessagePtr;
class ConnectionManager : public Network::ReadFilter,
                          public Network::ConnectionCallbacks,
                          public RequestDecoderCallbacks,
                          Logger::Loggable<Logger::Id::filter> {
public:
  ConnectionManager(Config& config, Random::RandomGenerator& random_generator,
                    TimeSource& time_system);
  ~ConnectionManager() override = default;

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  Network::FilterStatus onNewConnection() override;
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks&) override;

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent) override;
  void onAboveWriteBufferHighWatermark() override;
  void onBelowWriteBufferLowWatermark() override;

  // RequestDecoderCallbacks
  StreamHandler& newStream() override;
  void onHeartbeat(MetadataSharedPtr metadata) override;

  MetaProtocolProxyStats& stats() const { return stats_; }
  Network::Connection& connection() const { return read_callbacks_->connection(); }
  TimeSource& timeSystem() const { return time_system_; }
  Random::RandomGenerator& randomGenerator() const { return random_generator_; }
  Config& config() const { return config_; }

  void continueDecoding();
  void deferredMessage(ActiveMessage& message);
  void sendLocalReply(Metadata& metadata, const DirectResponse& response, bool end_stream);

  // This function is for testing only.
  std::list<ActiveMessagePtr>& getActiveMessagesForTest() { return active_message_list_; }

private:
  void dispatch();
  void resetAllMessages(bool local_reset);

  Buffer::OwnedImpl request_buffer_;
  std::list<ActiveMessagePtr> active_message_list_;

  bool stopped_{false};
  bool half_closed_{false};

  Config& config_;
  TimeSource& time_system_;
  MetaProtocolProxyStats& stats_;
  Random::RandomGenerator& random_generator_;

  CodecPtr codec_;
  RequestDecoderPtr decoder_;
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
