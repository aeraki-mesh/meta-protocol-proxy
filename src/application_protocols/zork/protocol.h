#pragma once
#include "source/common/buffer/buffer_impl.h"
#include "source/common/common/logger.h"
namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace MetaProtocolProxy {
namespace Zork {

#define MARKET_VS_FLAG 1
#define MARKET_VS_FLAG_COMPRESS 3
#define MARKET_VZ_FLAG 4
#define MARKET_VZ_FLAG_COMPRESS 6
#define MARKET_US_FLAG 5
#define MARKET_US_FLAG_COMPRESS 7
#define MARKET_UK_FLAG 8
#define MARKET_UK_FLAG_COMPRESS 10
#define MARKET_VK_FLAG 9
#define MARKET_VK_FLAG_COMPRESS 11
#define MARKET_DEFAULT_FLAG 99

enum class ZorkCode {
  NoRoute = 1,
  Error = 1,
};

//A protocol data first byte is '{' ,last byte is '}'
//,last byte don't add to pack_len,so packet len is header+pack_len+1.
struct ZorkHeader : public Logger::Loggable<Logger::Id::filter> {
  private:
    char tag_;
    uint16_t req_type_;
    uint16_t pack_attrs_;
    uint16_t pack_length_;
    //int32_t seq_id_;
    //uint16_t rsp_code_;

  public:
    static const uint16_t HEADER_SIZE;
    bool decode(Buffer::Instance& buffer);
    bool encode(Buffer::Instance& buffer);
    uint16_t get_tag() const {return tag_;};
    uint16_t get_req_type() const {return req_type_;};
    uint16_t get_pack_attrs() const {return pack_attrs_;};
    uint16_t get_pack_len() const {return pack_length_;};
    //int32_t get_seq_id() const {return seq_id_;};
    //uint16_t get_rsp_code() const {return rsp_code_;};

    void set_tag(char tag) {tag_ = tag;};
    void set_req_type(uint16_t req_type) {req_type_ = req_type;};
    void set_pack_attrs(uint16_t pack_attrs) {pack_attrs_ = pack_attrs;};
    void set_pack_len(uint16_t pack_len) {pack_length_ = pack_len;};
    //void set_seq_id(int32_t seq_id) {seq_id_ = seq_id;};
    //void set_rsp_code(uint16_t rsp_code) {rsp_code_ = rsp_code;};

};

} // namespace Zork
} // namespace MetaProtocolProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
