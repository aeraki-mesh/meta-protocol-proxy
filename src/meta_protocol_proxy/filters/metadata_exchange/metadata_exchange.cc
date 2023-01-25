#include "src/meta_protocol_proxy/filters/metadata_exchange/metadata_exchange.h"
#include "src/meta_protocol_proxy/app_exception.h"
#include "source/common/buffer/buffer_impl.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace MetadataExchange {

void MetadataExchangeFilter::onDestroy() { cleanup(); }

void MetadataExchangeFilter::setDecoderFilterCallbacks(DecoderFilterCallbacks& callbacks) {
  callbacks_ = &callbacks;
}

FilterStatus MetadataExchangeFilter::onMessageDecoded(MetadataSharedPtr metadata,
                                                      MutationSharedPtr) {
  return FilterStatus::ContinueIteration;
}

void MetadataExchangeFilter::setEncoderFilterCallbacks(EncoderFilterCallbacks& callbacks) {
  encoder_callbacks_ = &callbacks;
}

FilterStatus MetadataExchangeFilter::onMessageEncoded(MetadataSharedPtr, MutationSharedPtr) {
  return FilterStatus::ContinueIteration;
}

void MetadataExchangeFilter::cleanup() {}

} // namespace MetadataExchange
} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
