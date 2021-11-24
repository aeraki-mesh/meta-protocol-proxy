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
  // 表示从缓冲区读取数据的长度；如果值是-1/0则当前数据不完整
  int len = CVideoPacket::checkPacket(const_cast<char*>(buffer.toString().data()), buffer.length());

  if (len <= 0) {
    ENVOY_LOG(debug, "videopacket decoder: wait for data");
    return DecodeStatus::WaitForData;
  }

  std::string data = std::string(buffer.toString(), len);

  // video packet 协议解码
  CVideoPacket cvp;
  cvp.set_packet(reinterpret_cast<uint8_t*>(const_cast<char*>(buffer.toString().data())), len);
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
  // 从metadata 获取 command, 字符串类型
  int cmd = strtol(metadata.getString("command").c_str(), nullptr, 0);
  if (cmd <= 0) {
    cmd = 1;
  }
 
  CVideoPacket cvp;
  cvp.setCommand(cmd);

  switch (error.type) {
    case ErrorType::RouteNotFound: {
      std::cout << "Route Nod Found" << std::endl << std::endl << std::endl;
      cvp.setResult(videocomm::FAIL);
      break;
    }
    default: {
      std::cout << "Other" << std::endl << std::endl << std::endl;
      cvp.setResult(videocomm::FAIL);
      break;
    }
  }

  int ret = cvp.encode();
  if (ret != 0) {
    ENVOY_LOG(debug, "videopacket onError: encode failed ret = {}", ret);
  }

  // 写回buffer, 返回客户端的内容
  buffer.add(cvp.getPacket(), cvp.getLength());
}

void VideoPacketCodec::toMetadata(CVideoPacket& cvp, MetaProtocolProxy::Metadata& metadata,
      Buffer::Instance& buffer) {
    metadata.setRequestId(cvp.getReqUin());
    metadata.put("service_type",cvp.getServiceType());
    // metadata.put("command",cvp.getCommand());
    // 路由匹配需要 string 类型的 key-value
    metadata.putString("command", std::to_string(cvp.getCommand()));
    metadata.setOriginMessage(buffer);

    // 清空缓存
    buffer.drain(cvp.getPacketLen());
}

}   // namespace VideoPacket 
}   // namespace MetaProtocolProxy
}   // namespace NetworkFilters
}   // namespace Extensions
}   // namespace Envoy