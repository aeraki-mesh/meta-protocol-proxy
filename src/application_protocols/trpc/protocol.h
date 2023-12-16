#pragma once

#include <cstdint>
#include <string>

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"

#include "src/meta_protocol_proxy/codec/codec.h"
#include "src/application_protocols/trpc/metadata.h"
#include "src/application_protocols/trpc/trpc.pb.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

class DirectResponse : public Logger::Loggable<Logger::Id::filter> {
public:
  virtual ~DirectResponse() = default;

  // 用于统计
  enum class ResponseType {
    // DirectResponse encodes MessageType::Reply with success payload
    SuccessReply,

    // DirectResponse encodes MessageType::Reply with an empty payload
    ErrorReply,
  };

  /**
   * Encodes the response
   * @param metadata the MessageMetadata for the request that generated this response
   * @param buffer the Buffer into which the message should be encoded
   * @return ResponseType indicating whether the message is a successful or error reply
   */
  virtual ResponseType encode(MessageMetadata const& meta, Buffer::Instance& data) const = 0;

  virtual int32_t err_code() const = 0;
};

struct TrpcFixedHeader : public Logger::Loggable<Logger::Id::filter> {
  static const uint32_t TRPC_PROTO_PREFIX_SPACE;
  static const uint32_t TRPC_PROTO_MAGIC_SPACE;
  static const uint32_t TRPC_PROTO_DATAFRAME_TYPE_SPACE;
  static const uint32_t TRPC_PROTO_DATAFRAME_STATE_SPACE;
  static const uint32_t TRPC_PROTO_DATAFRAME_SIZE_SPACE;
  static const uint32_t TRPC_PROTO_HEADER_SIZE_SPACE;
  static const uint32_t TRPC_PROTO_STREAM_ID_SPACE;
  static const uint32_t TRPC_PROTO_REVERSED_SPACE;

  // 魔数
  uint16_t magic_value = trpc::TrpcMagic::TRPC_MAGIC_VALUE;

  // trpc协议的二进制数据帧类型
  // 计划支持两种类型的二进制数据帧：
  // 1. 一应一答模式的二进制数据帧类型
  // 2. 流式模式的二进制数据帧类型
  uint8_t data_frame_type = 0;

  // trpc协议的流式帧的类型
  uint8_t stream_frame_type = 0;

  // trpc协议请求体或响应体的二进制数据总大小
  // 由(固定头 + 请求包头或响应包头 + 业务序列化数据)的大小组成
  uint32_t data_frame_size = 0;

  // 请求包头或响应包头的大小
  uint16_t pb_header_size = 0;

  // 流id
  uint32_t stream_id = 0;

  // 保留字段
  char reserved[2] = {0};

  // tRPC协议头部固定帧头数据的解码
  bool decode(Buffer::Instance& buff, bool drain = true);

  // tRPC协议头部固定帧头数据的编码
  bool encode(Buffer::Instance& buffer) const;

  // 获取头部长度（帧头 + pb协议头）
  [[nodiscard]] uint32_t getHeaderSize() const { return pb_header_size + TRPC_PROTO_PREFIX_SPACE; }

  // 获取协议包体长度
  [[nodiscard]] uint32_t getPayloadSize() const { return data_frame_size - getHeaderSize(); }
};

template <typename T> class Protocol : public Logger::Loggable<Logger::Id::filter> {
public:
  Protocol() = default;
  virtual ~Protocol() = default;

  /**
   * 从协议中获取request id
   * @param req_id
   * @return
   */
  bool getRequestId(uint32_t& req_id) const {
    req_id = protocol_header_.request_id();
    return true;
  }

  /**
   * 设置协议的request id
   * @param req_id
   * @return
   */
  bool setRequestId(uint32_t req_id) {
    protocol_header_.set_request_id(req_id);
    return true;
  }

  /**
   * 协议请求的解码（只能解码一个完整的消息）
   * @param buff 存储待解码的消息
   * @return 成功则返回true，反之返回false
   */
  bool decode(Buffer::Instance& buff) {
    auto ptr_size = buff.length();

    if (!fixed_header_.decode(buff, true)) {
      return false;
    }

    if (ptr_size < fixed_header_.data_frame_size) {
      ENVOY_LOG(error, "decode ptr_size:{} < {}.", ptr_size, fixed_header_.data_frame_size);
      return false;
    }
    std::string header_raw;
    header_raw.reserve(fixed_header_.pb_header_size);
    header_raw.resize(fixed_header_.pb_header_size);
    buff.copyOut(TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE, fixed_header_.pb_header_size,
                 &(header_raw[0]));
    buff.drain(fixed_header_.pb_header_size);
    if (!protocol_header_.ParseFromString(header_raw)) {
      ENVOY_LOG(error, "decode req_header parse error.");
      return false;
    }

    body_.move(buff, static_cast<uint64_t>(fixed_header_.getPayloadSize()));

    return true;
  }

  /**
   * 协议请求的编码 (会计算固定帧头的pb_header_size和data_frame_size)
   * @param buff 存储编码后会的消息
   * @return 成功则返回true，反之返回false
   */
  bool encode(Buffer::Instance& buff) {
    // 计算长度
    fixed_header_.magic_value = trpc::TrpcMagic::TRPC_MAGIC_VALUE;
    fixed_header_.pb_header_size = protocol_header_.ByteSizeLong();
    fixed_header_.data_frame_size = fixed_header_.getHeaderSize() + body_.length();
    // encode
    fixed_header_.encode(buff);
    buff.add(protocol_header_.SerializeAsString());
    if (body_.length() > 0) {
      buff.add(body_);
    }
    return true;
  }

inline unsigned int to_uint(char ch)
{
    // EDIT: multi-cast fix as per David Hammen's comment
    return static_cast<unsigned int>(static_cast<unsigned char>(ch));
}
  /**
   * 修改 Header
   * @param buff
   */
  bool mutateHeader(Buffer::Instance& buff, const MetaProtocolProxy::Mutation& mutation) {
    auto ptr_size = buff.length();
 
    // 解析原始消息
    // 解析fix header
    if (!fixed_header_.decode(buff, true)) {
      return false;
    }

    if (ptr_size < fixed_header_.data_frame_size) {
      ENVOY_LOG(error, "decode ptr_size:{} < {}.", ptr_size, fixed_header_.data_frame_size);
      return false;
    }

    // 解析 protocol header
    std::string header_raw;
    header_raw.reserve(fixed_header_.pb_header_size);
    header_raw.resize(fixed_header_.pb_header_size);
    buff.copyOut(0, fixed_header_.pb_header_size,
                 &(header_raw[0]));
    buff.drain(fixed_header_.pb_header_size);
    if (!protocol_header_.ParseFromString(header_raw)) {
      ENVOY_LOG(error, "decode protocol header parse error.");
      return false;
    }

    // 保留 body
    Buffer::OwnedImpl body;
    body.move(buff, static_cast<uint64_t>(fixed_header_.getPayloadSize()));
    
    // 修改/添加消息头
    auto trans_info = protocol_header_.mutable_trans_info();
    for (const auto& keyValue : mutation) {
      (*trans_info)[keyValue.first] = keyValue.second;
    }
    
    // 修改 fixed header 中的协议头长度
    fixed_header_.pb_header_size = protocol_header_.ByteSizeLong();
    fixed_header_.data_frame_size = fixed_header_.getHeaderSize() + body.length();

    // 编码修改后的消息 
    fixed_header_.encode(buff);
    buff.add(protocol_header_.SerializeAsString());
    buff.add(body);

    return true;
  }

public:
  // tRPC协议头部固定帧头
  TrpcFixedHeader fixed_header_;
  // tRPC协议请求包头
  T protocol_header_;
  // 业务序列化的二进制数据
  Buffer::OwnedImpl body_;
};

using TrpcRequestProtocol = Protocol<trpc::RequestProtocol>;
using TrpcResponseProtocol = Protocol<trpc::ResponseProtocol>;
using TrpcStreamInitMeta = Protocol<trpc::TrpcStreamInitMeta>;

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

