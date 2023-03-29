#include "src/meta_protocol_proxy/upstream_response.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

void UpstreamHandlerResponseDecoder::onMessageDecoded(MetadataSharedPtr metadata,
                                                      MutationSharedPtr mutation) {
  // callback by request id

  complete_ = true;

  ENVOY_LOG(debug,
            "meta protocol response decoder: complete processing of upstream response messages, id is {}",
            metadata->getRequestId());
  codec_->encode(*metadata, *mutation, metadata->originMessage());
  handler_.onMessageDecoded(metadata, mutation);
};

void UpstreamResponse::startUpstreamResponse() {
  ENVOY_LOG(debug, "meta protocol response: start upstream");

  ASSERT(response_decoder_ == nullptr);

  CodecPtr codec = config_.createCodec();

  // Create a response message decoder.
  response_decoder_ = std::make_unique<UpstreamHandlerResponseDecoder>(handler_, std::move(codec));
}

UpstreamResponseStatus UpstreamResponse::upstreamData(Buffer::Instance& buffer) {
  ASSERT(response_decoder_ != nullptr);
  return response_decoder_->decode(buffer);
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
