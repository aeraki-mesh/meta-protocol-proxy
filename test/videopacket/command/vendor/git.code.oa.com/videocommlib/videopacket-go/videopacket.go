package videopacket

import (
	"bytes"
	"encoding/binary"
	"errors"

	"git.code.oa.com/jce/jce"
	"git.code.oa.com/videocommlib/videopacket-go/commhead"
)

const (
	// VideopacketStart is the start byte
	VideopacketStart = 0x26
	// VideopacketEnd is the end byte
	VideopacketEnd = 0x28
	// VideopacketMinLength is the minimize length of videopacket
	VideopacketMinLength = 17
)

// VideoPacket 定义了 videopacket 协议的格式
type VideoPacket struct {
	Start      byte
	Length     uint32
	Version    byte
	Reserve    [10]byte
	CommHeader commhead.VideoCommHeader
	End        byte
}

// NewVideoPacket will new a VideoPacket
func NewVideoPacket() *VideoPacket {
	return &VideoPacket{
		Start:   VideopacketStart,
		End:     VideopacketEnd,
		Version: 0x01,
	}
}

// Encode will encode a *VideoPacket to *bytes.Buffer
func (packet *VideoPacket) Encode() (*bytes.Buffer, error) {
	if packet == nil {
		return nil, errors.New("VideoPacket need init")
	}
	buffer := bytes.NewBuffer(nil)
	err := binary.Write(buffer, binary.LittleEndian, packet.Start)
	if err != nil {
		return nil, errors.New("VideoPacket Start write error:" + err.Error())
	}

	// Encode VideoCommHeader
	comm, err := jce.Marshal(&packet.CommHeader)
	if err != nil {
		return nil, errors.New("VideoCommHeader encode error:" + err.Error())
	}

	packet.Length = VideopacketMinLength + uint32(len(comm))
	err = binary.Write(buffer, binary.BigEndian, packet.Length)
	if err != nil {
		return nil, errors.New("VideoPacket Length write error:" + err.Error())
	}
	err = binary.Write(buffer, binary.LittleEndian, packet.Version)
	if err != nil {
		return nil, errors.New("VideoPacket Version write error:" + err.Error())
	}
	err = binary.Write(buffer, binary.LittleEndian, packet.Reserve)
	if err != nil {
		return nil, errors.New("VideoPacket Reserve write error:" + err.Error())
	}
	err = binary.Write(buffer, binary.LittleEndian, comm)
	if err != nil {
		return nil, errors.New("VideoPacket VideoCommHeader write error:" + err.Error())
	}
	err = binary.Write(buffer, binary.LittleEndian, packet.End)
	if err != nil {
		return nil, errors.New("VideoPacket End write error:" + err.Error())
	}
	return buffer, nil
}

// Decode will decode []byte to *VideoPacket
func (packet *VideoPacket) Decode(content []byte) error {
	buffer := bytes.NewBuffer(content)
	err := binary.Read(buffer, binary.LittleEndian, &packet.Start)
	if err != nil {
		return errors.New("VideoPacket Start read error:" + err.Error())
	}
	err = binary.Read(buffer, binary.BigEndian, &packet.Length)
	if err != nil {
		return errors.New("VideoPacket Length read error:" + err.Error())
	}
	err = binary.Read(buffer, binary.LittleEndian, &packet.Version)
	if err != nil {
		return errors.New("VideoPacket Start read error:" + err.Error())
	}
	err = binary.Read(buffer, binary.LittleEndian, &packet.Reserve)
	if err != nil {
		return errors.New("VideoPacket Start read error:" + err.Error())
	}

	comm := buffer.Next(int(packet.Length) - VideopacketMinLength)

	err = jce.Unmarshal(comm, &packet.CommHeader)
	if err != nil {
		return errors.New("VideoPacket VideoCommHeader decode error:" + err.Error())
	}

	err = binary.Read(buffer, binary.LittleEndian, &packet.End)
	if err != nil {
		return errors.New("VideoPacket Start read error:" + err.Error())
	}
	return nil
}

// CheckPacket will check whether []byte is a vaild *VideoPacket
// result is int when
// < 0 error
// = 0 recv not enough
// > 0 the real length of this packet
//
func CheckPacket(packetBuf []byte) int {
	packetLen := len(packetBuf)
	if packetLen < 17 {
		return 0
	}
	var stx uint8
	var msgLen uint32
	packetReader := bytes.NewReader(packetBuf)

	binary.Read(packetReader, binary.BigEndian, &stx)
	if stx != VideopacketStart {
		return -1
	}
	err := binary.Read(packetReader, binary.BigEndian, &msgLen)
	if err != nil {
		return -2
	}
	if packetLen < int(msgLen) {
		return 0
	}
	if packetBuf[0] != VideopacketStart || packetBuf[msgLen-1] != VideopacketEnd || msgLen < VideopacketMinLength {
		return -4
	}
	return int(msgLen)
}
