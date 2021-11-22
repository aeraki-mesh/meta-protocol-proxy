package codec

import (
	"bytes"
	"compress/zlib"
	"io/ioutil"
)

func init() {
	RegisterCompressor(CompressTypeZlib, &ZlibCompress{})
}

// ZlibCompress zlib解压缩
type ZlibCompress struct {
}

// Compress zlib压缩
func (c *ZlibCompress) Compress(in []byte) ([]byte, error) {

	if len(in) == 0 {
		return in, nil
	}

	var (
		buffer bytes.Buffer
		out    []byte
		err    error
	)
	writer := zlib.NewWriter(&buffer)
	_, err = writer.Write(in)
	if err != nil {
		writer.Close()
		return out, err
	}
	err = writer.Close()
	if err != nil {
		return out, err
	}

	return buffer.Bytes(), nil
}

// Decompress zlib解压缩
func (c *ZlibCompress) Decompress(in []byte) ([]byte, error) {

	if len(in) == 0 {
		return in, nil
	}

	reader, err := zlib.NewReader(bytes.NewReader(in))
	if err != nil {
		var out []byte
		return out, err
	}
	defer reader.Close()

	return ioutil.ReadAll(reader)
}
