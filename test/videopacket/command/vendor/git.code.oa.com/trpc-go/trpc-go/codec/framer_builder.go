package codec

import (
	"io"
)

// FramerBuilder 通常每个连接Build一个Framer
type FramerBuilder interface {
	New(io.Reader) Framer
}

// Framer 读写数据桢
type Framer interface {
	ReadFrame() ([]byte, error)
}
