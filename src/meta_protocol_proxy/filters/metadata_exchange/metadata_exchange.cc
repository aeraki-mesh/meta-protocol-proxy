#include "src/meta_protocol_proxy/filters/metadata_exchange/metadata_exchange.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "source/common/buffer/buffer_impl.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

FilterStatus MetadataExchangeFilter::onMessageDecoded(MetadataSharedPtr, MutationSharedPtr) {
  ENVOY_LOG(info, "xxxxxxxxxx meta protocol: metadata exchange ondecode");
  return FilterStatus::ContinueIteration;
}

FilterStatus MetadataExchangeFilter::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {

  ENVOY_LOG(info, "xxxxxxxxxxx meta protocol: metadata exchange onencode");
  return FilterStatus::ContinueIteration;
}

} // namespace MetadataExchange
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
