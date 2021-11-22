// 支持fastjce2go的底层库，用于基础类型的序列化
// 高级类型的序列化，由代码生成器，转换为基础类型的序列化
// author: cheneyhu@tencent.com

package jce

import (
	"bytes"
	"encoding/binary"
	"math"
	"unsafe"
)

type Buffer struct {
	buf *bytes.Buffer
}

func NewBuffer() *Buffer {
	return &Buffer{buf: &bytes.Buffer{}}
}

//go:nosplit
func bWriteU8(w *bytes.Buffer, data uint8) error {
	err := w.WriteByte(byte(data))
	return err
}

//go:nosplit
func bWriteU16(w *bytes.Buffer, data uint16) error {
	var b [2]byte
	var bs []byte
	bs = b[:]
	binary.BigEndian.PutUint16(bs, data)
	_, err := w.Write(bs)
	return err
}

//go:nosplit
func bWriteU32(w *bytes.Buffer, data uint32) error {
	var b [4]byte
	var bs []byte
	bs = b[:]
	binary.BigEndian.PutUint32(bs, data)
	_, err := w.Write(bs)
	return err
}

//go:nosplit
func bWriteU64(w *bytes.Buffer, data uint64) error {
	var b [8]byte
	var bs []byte
	bs = b[:]
	binary.BigEndian.PutUint64(bs, data)
	_, err := w.Write(bs)
	return err
}

//go:nosplit
func (b *Buffer) WriteHead(ty byte, tag byte) error {
	if tag < 15 {
		data := (tag << 4) | ty
		return b.buf.WriteByte(data)
	} else {
		data := (15 << 4) | ty
		if err := b.buf.WriteByte(data); err != nil {
			return err
		}
		return b.buf.WriteByte(tag)
	}
}

func (b *Buffer) Reset() {
	b.buf.Reset()
}

func (b *Buffer) Write_slice_uint8(data []uint8) error {
	_, err := b.buf.Write(data)
	return err
}

func (b *Buffer) Write_slice_int8(data []int8) error {
	_, err := b.buf.Write(*(*[]uint8)(unsafe.Pointer(&data)))
	return err
}

func (b *Buffer) Write_int8(data int8, tag byte) error {
	var err error
	if data == 0 {
		if err = b.WriteHead(ZERO_TAG, tag); err != nil {
			return err
		}
	} else {
		if err = b.WriteHead(BYTE, tag); err != nil {
			return err
		}

		if err = b.buf.WriteByte(byte(data)); err != nil {
			return err
		}
	}
	return nil
}

func (b *Buffer) Write_uint8(data uint8, tag byte) error {
	return b.Write_int16(int16(data), tag)
}

func (b *Buffer) Write_bool(data bool, tag byte) error {
	tmp := int8(0)
	if data {
		tmp = 1
	}
	return b.Write_int8(tmp, tag)
}

func (b *Buffer) Write_int16(data int16, tag byte) error {
	var err error
	if data >= math.MinInt8 && data <= math.MaxInt8 {
		if err = b.Write_int8(int8(data), tag); err != nil {
			return err
		}
	} else {
		if err = b.WriteHead(SHORT, tag); err != nil {
			return err
		}

		if err = bWriteU16(b.buf, uint16(data)); err != nil {
			return err
		}
	}
	return nil
}

func (b *Buffer) Write_uint16(data uint16, tag byte) error {
	return b.Write_int32(int32(data), tag)
}

func (b *Buffer) Write_int32(data int32, tag byte) error {
	var err error
	if data >= math.MinInt16 && data <= math.MaxInt16 {
		if err = b.Write_int16(int16(data), tag); err != nil {
			return err
		}
	} else {
		if err = b.WriteHead(INT, tag); err != nil {
			return err
		}

		if err = bWriteU32(b.buf, uint32(data)); err != nil {
			return err
		}
	}
	return nil
}

func (b *Buffer) Write_uint32(data uint32, tag byte) error {
	return b.Write_int64(int64(data), tag)
}

func (b *Buffer) Write_int64(data int64, tag byte) error {
	var err error
	if data >= math.MinInt32 && data <= math.MaxInt32 {
		if err = b.Write_int32(int32(data), tag); err != nil {
			return err
		}
	} else {
		if err = b.WriteHead(LONG, tag); err != nil {
			return err
		}

		if err = bWriteU64(b.buf, uint64(data)); err != nil {
			return err
		}
	}
	return nil
}

func (b *Buffer) Write_float32(data float32, tag byte) error {
	var err error
	if err = b.WriteHead(FLOAT, tag); err != nil {
		return err
	}

	if err = bWriteU32(b.buf, math.Float32bits(data)); err != nil {
		return err
	}
	return nil
}

func (b *Buffer) Write_float64(data float64, tag byte) error {
	var err error
	if err = b.WriteHead(DOUBLE, tag); err != nil {
		return err
	}

	if err = bWriteU64(b.buf, math.Float64bits(data)); err != nil {
		return err
	}
	return nil
}

func (b *Buffer) Write_string(data string, tag byte) error {
	var err error
	if len(data) > 255 {
		if err = b.WriteHead(byte(STRING4), tag); err != nil {
			return err
		}

		if err = bWriteU32(b.buf, uint32(len(data))); err != nil {
			return err
		}
	} else {
		if err = b.WriteHead(byte(STRING1), tag); err != nil {
			return err
		}

		if err = bWriteU8(b.buf, byte(len(data))); err != nil {
			return err
		}
	}

	if _, err = b.buf.WriteString(data); err != nil {
		return err
	}
	return nil
}

func (b *Buffer) ToBytes() []byte {
	return b.buf.Bytes()
}

func (b *Buffer) Grow(size int) {
	b.buf.Grow(size)
}

