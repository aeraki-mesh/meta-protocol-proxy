#pragma once

#include "envoy/tcp/conn_pool.h"
#include "envoy/upstream/thread_local_cluster.h"

#include "src/meta_protocol_proxy/filters/filter.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Router {

/**
 * This interface is used by an upstream request to communicate its state.
 */
class RequestOwner {
public:
  virtual ~RequestOwner() = default;
  /**
   * @return ConnectionPool::UpstreamCallbacks& the handler for upstream data.
   */
  virtual Tcp::ConnectionPool::UpstreamCallbacks& upstreamCallbacks() PURE;

  /*
   * @return DecoderFilterCallbacks
   */
  virtual DecoderFilterCallbacks& decoderFilterCallbacks() PURE;

  /*
   * @return EncoderFilterCallbacks
   */
  virtual EncoderFilterCallbacks& encoderFilterCallbacks() PURE;
};

} // namespace Router
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
