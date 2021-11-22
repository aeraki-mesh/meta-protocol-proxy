package codec

import (
	"errors"
)

// Serializer body序列化接口
type Serializer interface {
	Unmarshal(in []byte, body interface{}) error
	Marshal(body interface{}) (out []byte, err error)
}

// SerializationType content body序列化方式
// protobuf jce json http-get-query http-get-restful
// 目前约定0-127范围的取值为trpc所有语言框架规范的共同的序列化方式，框架使用
// 128-999 为各语言独立指定的序列化方式，框架使用
// 业务自定义序列化值必须从1000开始，最好更分散一点，避免各业务间相互冲突
const (
	SerializationTypePB         = 0 // protobuf
	SerializationTypeJCE        = 1 // jce
	SerializationTypeJSON       = 2 // json
	SerializationTypeFlatBuffer = 3 // flat buffer
	SerializationTypeNoop       = 4 // bytes 二进制数据空序列化方式

	SerializationTypeUnsupported = 128 // 不支持
	SerializationTypeForm        = 129 // http form data 表单kv结构
	SerializationTypeGet         = 130 // http server 处理get请求
)

var serializers = make(map[int]Serializer)

// RegisterSerializer 注册序列化具体实现Serializer，由第三方包的init函数调用
func RegisterSerializer(serializationType int, s Serializer) {
	serializers[serializationType] = s
}

// GetSerializer 通过serialization type获取Serializer
func GetSerializer(serializationType int) Serializer {
	return serializers[serializationType]
}

// Unmarshal 解析body，内部通过不同serialization type，调用不同的序列化方式，默认protobuf
func Unmarshal(serializationType int, in []byte, body interface{}) error {

	if body == nil {
		return nil
	}

	if len(in) == 0 {
		return nil
	}

	if serializationType == SerializationTypeUnsupported {
		return nil
	}

	s := GetSerializer(serializationType)
	if s == nil {
		return errors.New("serializer not registered")
	}

	return s.Unmarshal(in, body)
}

// Marshal 打包body，内部通过不同serialization type，调用不同的序列化方式, 默认protobuf
func Marshal(serializationType int, body interface{}) ([]byte, error) {

	if body == nil {
		return nil, nil
	}

	if serializationType == SerializationTypeUnsupported {
		return nil, nil
	}

	s := GetSerializer(serializationType)
	if s == nil {
		return nil, errors.New("serializer not registered")
	}

	return s.Marshal(body)
}
