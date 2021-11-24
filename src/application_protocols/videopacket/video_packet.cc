
#include "src/application_protocols/videopacket/video_packet.h"

const uint16_t CVideoPacket::MIN_PACKET_LEN = 17;

CVideoPacket::CVideoPacket()
        : m_wVideoCommHeaderLen(0)
        , m_cStx(VIDEO_STX)
        , m_dwPacketLen(0)
        , m_cVersion(0x01)
        , m_cEtx(VIDEO_ETX)
        , packet(nullptr)
{
    /*
    MIN_PACKET_LEN = sizeof(m_cStx);
    MIN_PACKET_LEN += sizeof(m_dwPacketLen);
    MIN_PACKET_LEN += sizeof(m_cVersion);
    MIN_PACKET_LEN += sizeof(m_acReserved);
    MIN_PACKET_LEN += sizeof(m_cEtx);
    */
    memset( m_acReserved, 0, sizeof(m_acReserved) );
}

CVideoPacket::~CVideoPacket() {
    delBuf();
}

int CVideoPacket::decode() {
    if ( m_dwPacketLen < MIN_PACKET_LEN ) {
        return -1;
    }
    // 1. decode 头
    uint8_t *buffer = packet;
    m_cStx = *buffer;
    buffer += sizeof(m_cStx);
    m_dwPacketLen = htonl(*reinterpret_cast<uint32_t *>(buffer));
    buffer += sizeof(m_dwPacketLen);
    m_cVersion = *buffer;
    buffer += sizeof(m_cVersion);
    memcpy(m_acReserved, buffer, sizeof(m_acReserved));
    buffer += sizeof(m_acReserved);

    // 2. decode 尾
    m_cEtx = packet[m_dwPacketLen-1];

    // 3. decode commHeader
    if ( 0 == is_valid_packet() ) {
        return -2;
    }

    m_wVideoCommHeaderLen = m_dwPacketLen - MIN_PACKET_LEN;
    // 解析commHeader
    try {
        taf::JceInputStream<taf::BufferReader> isJce;
        isJce.setBuffer(reinterpret_cast<char*>(buffer), m_wVideoCommHeaderLen);
        m_stVideoCommHeader.readFrom(isJce);
    } catch(std::exception & e) {
        return -1;
    }
    buffer += m_wVideoCommHeaderLen;
    return 0;
}

int CVideoPacket::encode() {
    taf::JceOutputStream<taf::BufferWriter> osJce;
    try {
        m_stVideoCommHeader.writeTo(osJce);
    } catch(std::exception & e) {
        return -1;
    }
    m_wVideoCommHeaderLen = osJce.getLength();
    m_dwPacketLen = m_wVideoCommHeaderLen + MIN_PACKET_LEN;
    if ( allocBuf( m_dwPacketLen ) ) {
        return -2;
    }

    uint8_t* buffer = packet;
    *buffer = m_cStx;
    buffer += sizeof(m_cStx);
    *(reinterpret_cast<uint32_t *>(buffer)) = htonl(m_dwPacketLen);
    buffer += sizeof(m_dwPacketLen);
    *buffer = m_cVersion;
    buffer += sizeof(m_cVersion);
    memcpy(buffer, m_acReserved, sizeof(m_acReserved));
    buffer += sizeof(m_acReserved);
    memcpy(buffer, osJce.getBuffer(), osJce.getLength());
    buffer += osJce.getLength();
    *buffer = m_cEtx;
    buffer += sizeof(m_cEtx);

    return 0;
}

int CVideoPacket::set_packet(uint8_t *pData, uint32_t wDataLen) {
    if (allocBuf(wDataLen)) {
        return -3;
    }
    memcpy(packet, pData, wDataLen);
    m_dwPacketLen = wDataLen;
    return 0;
}
