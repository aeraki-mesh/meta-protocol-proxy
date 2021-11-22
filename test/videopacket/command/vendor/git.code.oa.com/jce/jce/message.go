package jce

// Message jce消息体接口，类似protobuf
type Message interface {
	ReadFrom(_is *Reader) error
	WriteTo(_os *Buffer) error
}

// Marshal jce打包,与标准包 json xml proto 保持一致
func Marshal(msg Message) ([]byte, error) {
	buf := NewBuffer()
	msg.WriteTo(buf)
	return buf.ToBytes(), nil
}

// Unmarshal jce解包
func Unmarshal(data []byte, msg Message) error {
	buf := NewReader(data)
	return msg.ReadFrom(buf)
}
