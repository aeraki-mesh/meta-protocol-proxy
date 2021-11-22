package codec

import (
	"bytes"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
)

func init() {
	RegisterSerializer(SerializationTypeJSON, &JSONPBSerialization{})
}

// Marshaler jsonpb序列化结构体，可自己更改参数
var Marshaler = jsonpb.Marshaler{EmitDefaults: true, OrigName: true, EnumsAsInts: true}

// Unmarshaler jsonpb反序列化结构体，可自己更改参数
var Unmarshaler = jsonpb.Unmarshaler{AllowUnknownFields: true}

// JSONPBSerialization 序列化json包体，基于protobuf/jsonpb
type JSONPBSerialization struct{}

// Unmarshal 反序列json
func (s *JSONPBSerialization) Unmarshal(in []byte, body interface{}) error {
	input, ok := body.(proto.Message)
	if !ok {
		return JSONAPI.Unmarshal(in, body)
	}
	return Unmarshaler.Unmarshal(bytes.NewReader(in), input)
}

// Marshal 序列化json
func (s *JSONPBSerialization) Marshal(body interface{}) ([]byte, error) {
	input, ok := body.(proto.Message)
	if !ok {
		return JSONAPI.Marshal(body)
	}

	buf := []byte{}
	w := bytes.NewBuffer(buf)
	err := Marshaler.Marshal(w, input)
	if err != nil {
		return nil, err
	}
	return w.Bytes(), nil
}
