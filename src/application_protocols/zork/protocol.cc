#include "src/application_protocols/zork/protocol.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Zork {

const uint16_t ZorkHeader::HEADER_SIZE = 7;

bool ZorkHeader::decode(Buffer::Instance& buffer) {
  if (buffer.length() < HEADER_SIZE) {
    ENVOY_LOG(error, "Zork Header decode buffer.length:{} < {}.", buffer.length(), HEADER_SIZE);
    return false;
  }

  uint64_t pos = 0;
  tag_ = buffer.peekLEInt<int8_t>(pos);
  pos += sizeof(int8_t);
  req_type_ = buffer.peekLEInt<uint16_t>(pos);
  pos += sizeof(uint16_t);
  pack_attrs_ = buffer.peekLEInt<uint16_t>(pos);
  pos += sizeof(uint16_t);
  pack_length_ = buffer.peekLEInt<uint16_t>(pos);
  pos += sizeof(uint16_t);

  //seq_id_ = buffer.peekLEInt<int32_t>(pos);
  

  switch (pack_attrs_)
  {
  case MARKET_VS_FLAG:
    pack_attrs_ = MARKET_VS_FLAG; 
    break;
  case MARKET_VS_FLAG_COMPRESS:
    pack_attrs_ = MARKET_VS_FLAG_COMPRESS;
    break;
  case MARKET_VZ_FLAG:
    pack_attrs_ = MARKET_VZ_FLAG;
    break;
  case MARKET_VZ_FLAG_COMPRESS:
    pack_attrs_ = MARKET_VZ_FLAG_COMPRESS;
    break;
  case MARKET_US_FLAG:
    pack_attrs_ = MARKET_US_FLAG;
    break;
  case MARKET_US_FLAG_COMPRESS:
    pack_attrs_ = MARKET_US_FLAG_COMPRESS;
    break;
  case MARKET_UK_FLAG:
    pack_attrs_ = MARKET_UK_FLAG;
    break;
  case MARKET_UK_FLAG_COMPRESS:
    pack_attrs_ = MARKET_UK_FLAG_COMPRESS;
    break;
  case MARKET_VK_FLAG:
    pack_attrs_ = MARKET_VK_FLAG;
    break;
  case MARKET_VK_FLAG_COMPRESS:
    pack_attrs_ = MARKET_VK_FLAG_COMPRESS;
    break;
  default:
    pack_attrs_ = MARKET_DEFAULT_FLAG;
    break;
  }
  //for debug little Endian
  ENVOY_LOG(debug, "Zork Header tag_-{} req_type_-{} pack_attrs_-{} pack_length-{} pos-{}",tag_,req_type_,pack_attrs_,pack_length_,pos);
  
  ASSERT(pos == HEADER_SIZE);

  return true;
}

bool ZorkHeader::encode(Buffer::Instance& buffer) {
    // buffer.writeLEInt<int8_t>(tag_);
    // buffer.writeLEInt<uint16_t>(req_type_);
    // buffer.writeLEInt<uint16_t>(pack_attrs_);
    // buffer.writeLEInt<uint16_t>(sizeof(uint16_t));
    // buffer.writeLEInt<uint16_t>(rsp_code_);
  (void)buffer; 
  return true;
}

} // namespace Awesomerpc
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy