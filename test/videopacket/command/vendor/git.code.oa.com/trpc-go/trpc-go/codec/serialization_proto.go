package codec

import (
	"errors"

	"github.com/golang/protobuf/proto"
)

func init() {
	RegisterSerializer(SerializationTypePB, &PBSerialization{})
}

// PBSerialization 序列化protobuf包体
type PBSerialization struct{}

// Unmarshal 反序列protobuf
func (s *PBSerialization) Unmarshal(in []byte, body interface{}) error {
	msg, ok := body.(proto.Message)
	if !ok {
		return errors.New("unmarshal fail: body not protobuf message")
	}
	return proto.Unmarshal(in, msg)
}

// Marshal 序列化protobuf
func (s *PBSerialization) Marshal(body interface{}) ([]byte, error) {
	msg, ok := body.(proto.Message)
	if !ok {
		return nil, errors.New("marshal fail: body not protobuf message")
	}
	return proto.Marshal(msg)
}
