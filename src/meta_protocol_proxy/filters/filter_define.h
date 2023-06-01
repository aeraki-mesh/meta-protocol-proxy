#pragma once

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

enum class UpstreamResponseStatus : uint8_t {
  MoreData = 0, // The upstream response requires more data.
  Complete = 1, // The upstream response is complete.
  Reset = 2,    // The upstream response is invalid and its connection must be reset.
  Retry = 3, // The upstream response is failure need to retry. TODO not used, may remove it later
};

} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
