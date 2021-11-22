package codec

func init() {
	RegisterCompressor(CompressTypeNoop, &NoopCompress{})
}

// NoopCompress 空实现解压缩
type NoopCompress struct {
}

// Compress 空压缩，返回原始内容
func (c *NoopCompress) Compress(in []byte) (out []byte, err error) {
	return in, nil
}

// Decompress 空解压，返回原始内容
func (c *NoopCompress) Decompress(in []byte) (out []byte, err error) {
	return in, nil
}
