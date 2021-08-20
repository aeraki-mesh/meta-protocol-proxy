#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/meta_protocol_proxy/heartbeat_response.h"
#include "src/meta_protocol_proxy/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {

DirectResponse::ResponseType HeartbeatResponse::encode(Metadata& metadata, Codec& codec,
                                                       Buffer::Instance& buffer) const {
  metadata.setMessageType(MessageType::Heartbeat);
  codec.encode(metadata, MutationImpl{}, buffer);
  ENVOY_LOG(debug, "buffer length {}", buffer.length());
  return DirectResponse::ResponseType::SuccessReply;
}

} // namespace  MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
