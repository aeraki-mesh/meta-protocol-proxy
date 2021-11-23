#include "src/application_protocols/videopacket/videopacket_codec.h"
#include <any>
#include "envoy/buffer/buffer.h"
#include "common/common/logger.h"
#include "src/meta_protocol_proxy/codec/codec.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace VideoPacket {

MetaProtocolProxy::DecodeStatus VideoPacketCodec::decode(Buffer::Instance& buffer,
                                                  MetaProtocolProxy::Metadata& metadata) {
  ENVOY_LOG(debug, "videopacket decoder: {} bytes available", buffer.length());
  // https://km.woa.com/articles/show/311763?kmref=search&from_page=1&no=1
  int hasfinished;
  // 表示是否还需要再从缓冲区读取数据；如果值是-1/0则当前数据不完整
  hasfinished = CVideoPacket::checkPacket(const_cast<char*>(buffer.toString().data()), buffer.length());

  if (hasfinished <= 0) {
    ENVOY_LOG(debug, "videopacket decoder: wait for data");
    return DecodeStatus::WaitForData;
  }
  CVideoPacket cvp;
  cvp.set_packet(reinterpret_cast<uint8_t*>(buffer.toString().data()), buffer.length());
  cvp.decode();

  toMetadata(cvp, metadata, buffer);

  return DecodeStatus::Done;
}

void VideoPacketCodec::encode(const MetaProtocolProxy::Metadata& metadata,
                       const MetaProtocolProxy::Mutation& mutation, Buffer::Instance& buffer) {
  // TODO
  (void)metadata;
  (void)mutation;
  (void)buffer;
}

void VideoPacketCodec::onError(const MetaProtocolProxy::Metadata& metadata,
                        const MetaProtocolProxy::Error& error, Buffer::Instance& buffer) {
  (void)metadata;
  (void)error;
  (void)buffer;
  // FIXME: have been released in cvp.decode()
}

void VideoPacketCodec::toMetadata(CVideoPacket& cvp, MetaProtocolProxy::Metadata& metadata, Buffer::Instance& buffer) {
    metadata.setRequestId(cvp.getReqUin());
    metadata.put("service_type",cvp.getServiceType());
    metadata.put("command",cvp.getCommand());

    metadata.setOriginMessage(buffer);
}

}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy