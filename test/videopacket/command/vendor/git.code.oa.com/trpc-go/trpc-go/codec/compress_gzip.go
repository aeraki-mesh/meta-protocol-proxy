package codec

import (
	"bytes"
	"compress/gzip"
	"io/ioutil"
)

func init() {
	RegisterCompressor(CompressTypeGzip, &GzipCompress{})
}

// GzipCompress gzip解压缩
type GzipCompress struct {
}

// Compress gzip压缩
func (c *GzipCompress) Compress(in []byte) (out []byte, err error) {

	if len(in) == 0 {
		return in, nil
	}

	var buffer bytes.Buffer
	writer := gzip.NewWriter(&buffer)
	_, err = writer.Write(in)
	if err != nil {
		_ = writer.Close()
		return nil, err
	}
	err = writer.Close()
	if err != nil {
		return nil, err
	}

	return buffer.Bytes(), nil
}

// Decompress gzip解压
func (c *GzipCompress) Decompress(in []byte) (out []byte, err error) {

	if len(in) == 0 {
		return in, nil
	}

	reader, err := gzip.NewReader(bytes.NewReader(in))
	if err != nil {
		return nil, err
	}
	out, err = ioutil.ReadAll(reader)
	if err != nil {
		_ = reader.Close()
		return nil, err
	}
	err = reader.Close()
	if err != nil {
		return nil, err
	}

	return out, nil
}
