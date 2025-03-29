
#include "src/application_protocols/trpc/protocol.h"

namespace {

// 使用函数模板类型推导，防止手写出现类型不匹配
template <typename T>
inline void getIntFromInstance(T* t, Envoy::Buffer::Instance& buff, uint64_t pos) {
  *t = buff.peekBEInt<T>(pos);
}

template <typename T>
inline void writeIntToInstance(T* t, Envoy::Buffer::Instance& buff) {
  buff.writeBEInt<T>(*t);
}
}

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Trpc {

const uint32_t TrpcFixedHeader::TRPC_PROTO_PREFIX_SPACE = 16;
const uint32_t TrpcFixedHeader::TRPC_PROTO_MAGIC_SPACE = 2;
const uint32_t TrpcFixedHeader::TRPC_PROTO_DATAFRAME_TYPE_SPACE = 1;
const uint32_t TrpcFixedHeader::TRPC_PROTO_DATAFRAME_STATE_SPACE = 1;
const uint32_t TrpcFixedHeader::TRPC_PROTO_DATAFRAME_SIZE_SPACE = 4;
const uint32_t TrpcFixedHeader::TRPC_PROTO_HEADER_SIZE_SPACE = 2;
const uint32_t TrpcFixedHeader::TRPC_PROTO_STREAM_ID_SPACE = 4;
const uint32_t TrpcFixedHeader::TRPC_PROTO_REVERSED_SPACE = 2;

bool TrpcFixedHeader::decode(Buffer::Instance& buff, bool drain) {
  if (buff.length() < TRPC_PROTO_PREFIX_SPACE) {
    ENVOY_LOG(debug, "decode buff.length:{} < {}.", buff.length(), TRPC_PROTO_PREFIX_SPACE);
    return false;
  }
  // get magic_value
  uint64_t pos = 0;
  getIntFromInstance(&magic_value, buff, pos);
  // 非trpc协议,断开连接
  if (magic_value != trpc::TrpcMagic::TRPC_MAGIC_VALUE) {
    ENVOY_LOG(debug, "decode magic_value:{} != {}.", magic_value,
              trpc::TrpcMagic::TRPC_MAGIC_VALUE);
    return false;
  }
  pos += TRPC_PROTO_MAGIC_SPACE;
  // get data_frame_type
  getIntFromInstance(&data_frame_type, buff, pos);
  pos += TRPC_PROTO_DATAFRAME_TYPE_SPACE;
  // get stream_frame_type
  getIntFromInstance(&stream_frame_type, buff, pos);
  pos += TRPC_PROTO_DATAFRAME_STATE_SPACE;
  // get data_frame_size
  getIntFromInstance(&data_frame_size, buff, pos);
  pos += TRPC_PROTO_DATAFRAME_SIZE_SPACE;
  // get pb_header_size
  getIntFromInstance(&pb_header_size, buff, pos);
  pos += TRPC_PROTO_HEADER_SIZE_SPACE;
  // get stream_id
  getIntFromInstance(&stream_id, buff, pos);
  pos += TRPC_PROTO_STREAM_ID_SPACE;
  // get reserved
  buff.copyOut(pos, sizeof reserved, reserved);
  pos += TRPC_PROTO_REVERSED_SPACE;

  if (drain) {
    buff.drain(pos);
  }
  return true;
}

bool TrpcFixedHeader::encode(Buffer::Instance& buffer) const {
  writeIntToInstance(&magic_value, buffer);
  writeIntToInstance(&data_frame_type, buffer);
  writeIntToInstance(&stream_frame_type, buffer);
  writeIntToInstance(&data_frame_size, buffer);
  writeIntToInstance(&pb_header_size, buffer);
  writeIntToInstance(&stream_id, buffer);
  buffer.add(reserved, sizeof reserved);
  return true;
}

} // namespace Trpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
