package codec

import (
	"errors"

	"git.code.oa.com/jce/jce"
)

func init() {
	RegisterSerializer(SerializationTypeJCE, &JCESerialization{})
}

// JCESerialization 序列化 jce 包体
type JCESerialization struct{}

// Unmarshal 反序列化 jce
func (j *JCESerialization) Unmarshal(in []byte, body interface{}) error {
	if _, ok := body.(jce.Message); !ok {
		return errors.New("not jce.Message")
	}
	return jce.Unmarshal(in, body.(jce.Message))
}

// Marshal 序列化 jce
func (j *JCESerialization) Marshal(body interface{}) (out []byte, err error) {
	if _, ok := body.(jce.Message); !ok {
		return nil, errors.New("not jce.Message")
	}
	return jce.Marshal(body.(jce.Message))
}
