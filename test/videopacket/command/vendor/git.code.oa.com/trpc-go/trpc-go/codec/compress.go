package codec

import (
	"errors"
)

// Compressor body解压缩接口
type Compressor interface {
	Compress(in []byte) (out []byte, err error)
	Decompress(in []byte) (out []byte, err error)
}

// CompressType content body解压缩方式
const (
	CompressTypeNoop   = 0
	CompressTypeGzip   = 1
	CompressTypeSnappy = 2
	CompressTypeZlib   = 3
)

var compressors = make(map[int]Compressor)

// RegisterCompressor 注册解压缩具体实现，由第三方包的init函数调用
func RegisterCompressor(compressType int, s Compressor) {
	compressors[compressType] = s
}

// GetCompressor 通过compress type获取Compressor
func GetCompressor(compressType int) Compressor {
	return compressors[compressType]
}

// Compress 通过不同的压缩方式来压缩
func Compress(compressorType int, in []byte) (out []byte, err error) {
	if len(in) == 0 {
		return nil, nil
	}
	compressor := GetCompressor(compressorType)
	if compressor == nil {
		return nil, errors.New("compressor not registered")
	}
	return compressor.Compress(in)
}

// Decompress 通过不同的压缩方式来解压
func Decompress(compressorType int, in []byte) (out []byte, err error) {
	if len(in) == 0 {
		return nil, nil
	}
	compressor := GetCompressor(compressorType)
	if compressor == nil {
		return nil, errors.New("compressor not registered")
	}
	return compressor.Decompress(in)
}
