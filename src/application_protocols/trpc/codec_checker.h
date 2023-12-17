
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
#include "envoy/network/filter.h"
#include "envoy/server/filter_config.h"

#include "src/application_protocols/trpc/protocol.h"
#include "src/application_protocols/trpc/trpc.pb.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

class CodecCheckerCallBacks {
public:
  virtual ~CodecCheckerCallBacks() = default;

  /**
   * 当帧头解析成功后，会回调该函数。
   * @param fixed_header_ptr 帧头
   */
  virtual void onFixedHeaderDecoded(std::unique_ptr<TrpcFixedHeader> /*fixed_header_ptr*/) {}

  /**
   * 当数据满足包头长度后，会回调该函数，用户需要自己执行pb的反序列化。
   * @param str 存储包头序列化后的二进制数据。
   * @return
   */
  virtual bool onUnaryHeader(std::string&& /*str*/) { return true; }

  /**
   * 当数据满足流式帧长度后，会回调该函数，用户需要自己执行pb的反序列化。
   * @param str 存储包头序列化后的二进制数据。
   * @return
   */
  virtual bool onStreamFrame(std::string&& /*str*/) { return true; }

  /**
   * 当一个完整的数据包被接收后，返回整个包的原始数据。
   * @param msg 存储一个完整的原始数据包(包括帧头、包头、包体)。
   */
  virtual void onCompleted(std::unique_ptr<Buffer::OwnedImpl> /*msg*/) {}
};

// 数据包检查类
class CodecChecker : public Logger::Loggable<Logger::Id::filter> {
public:
  // tRPC协议 = 帧头 + 包头 + 包体
  // kDecodeFixedHeader 解析帧头
  // kDecodeUnaryProtocolHeader 解析一元请求包头
  // KDecodeStreamFrame 解析流式请求
  // kDecodePayload 解析包体
  enum class DecodeStage {
    kDecodeFixedHeader,
    kDecodeUnaryProtocolHeader,
    KDecodeStreamFrame,
    kDecodePayload,
    kDecodeDone,
    kWaitForData
  };

public:
  explicit CodecChecker(CodecCheckerCallBacks& call_backs) : call_backs_(call_backs) {}
  ~CodecChecker() = default;

  /**
   * 对外提供的接口，如果不是trpc协议，则throw EnvoyException。
   * @param data 输入数据。
   * @return DecodeStage
   */
  DecodeStage onData(Buffer::Instance& data);

private:
  /**
   *
   * @param buffer 输入数据
   * @return
   */
  DecodeStage handleState(Buffer::Instance& buffer);

  /**
   * 检查帧头。
   * @param buffer 输入数据
   * @return
   */
  DecodeStage decodeFixedHeader(Buffer::Instance& buffer);

  /**
   * 检查一元调用包头。
   * @param buffer 输入数据
   * @return
   */
  DecodeStage decodeUnaryProtocolHeader(Buffer::Instance& buffer);

  /**
   * 检查流式调用。
   * @param buffer 输入数据
   * @return
   */
  DecodeStage decodeStreamFrame(Buffer::Instance& buffer);

  /**
   * 检查包体。
   * @param buffer 输入数据
   * @return
   */
  DecodeStage decodePayload(Buffer::Instance& buffer);

  /**
   * 重置状态；执行后，才可以进行下一个包的解析。
   */
  void reset() {
    decode_stage_ = DecodeStage::kDecodeFixedHeader;
    total_size_ = 0;
    protocol_header_size_ = 0;
  }

private:
  // 状态
  DecodeStage decode_stage_{DecodeStage::kDecodeFixedHeader};
  // 总大小
  uint32_t total_size_{0};
  // 包头大小
  uint16_t protocol_header_size_{0};
  // 回调函数
  CodecCheckerCallBacks& call_backs_;
};

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
