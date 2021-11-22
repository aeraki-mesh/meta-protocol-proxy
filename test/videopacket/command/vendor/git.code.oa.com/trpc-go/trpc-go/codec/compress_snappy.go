package codec

import (
	"bytes"
	"io/ioutil"

	"github.com/golang/snappy"
)

func init() {
	RegisterCompressor(CompressTypeSnappy, &SnappyCompress{})
}

// SnappyCompress snappy解压缩
type SnappyCompress struct {
}

// Compress 压缩
func (c *SnappyCompress) Compress(in []byte) (out []byte, err error) {

	if len(in) == 0 {
		return in, nil
	}

	var buffer bytes.Buffer
	writer := snappy.NewWriter(&buffer)
	_, err = writer.Write(in)
	if err != nil {
		writer.Close()
		return nil, err
	}
	err = writer.Close()
	if err != nil {
		return nil, err
	}

	return buffer.Bytes(), nil
}

// Decompress 解压
func (c *SnappyCompress) Decompress(in []byte) (out []byte, err error) {

	if len(in) == 0 {
		return in, nil
	}

	reader := snappy.NewReader(bytes.NewReader(in))
	out, err = ioutil.ReadAll(reader)
	if err != nil {
		return nil, err
	}

	return out, err
}
