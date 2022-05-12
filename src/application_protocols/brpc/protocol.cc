#include "src/application_protocols/brpc/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Brpc {

const uint32_t BrpcHeader::HEADER_SIZE = 12;
const uint32_t BrpcHeader::MAGIC = *static_cast<uint32_t*>(static_cast<void*>((const_cast<char*>("PRPC"))));

bool BrpcHeader::decode(Buffer::Instance& buffer) {
  if (buffer.length() < HEADER_SIZE) {
    ENVOY_LOG(error, "Brpc Header decode buffer.length:{} < {}.", buffer.length(), HEADER_SIZE);
    return false;
  }

  uint32_t pos = 0;

  // todo : Check MAGIC
  pos += sizeof(uint32_t);

  _body_len = buffer.peekBEInt<uint32_t>(pos);
  pos += sizeof(uint32_t);
  _meta_len = buffer.peekBEInt<uint32_t>(pos);
  pos += sizeof(uint32_t);

  ASSERT(pos == HEADER_SIZE);

  return true;
}

bool BrpcHeader::encode(Buffer::Instance& buffer) {
  buffer.writeBEInt(MAGIC);
  buffer.writeBEInt(_body_len);
  buffer.writeBEInt(_meta_len);
  return true;
}

} // namespace Brpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
