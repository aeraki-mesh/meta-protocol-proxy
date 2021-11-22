package jce

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"io"
	"math"
	"unsafe"
)

//jce type
const (
	BYTE byte = iota
	SHORT
	INT
	LONG
	FLOAT
	DOUBLE
	STRING1
	STRING4
	MAP
	LIST
	STRUCT_BEGIN
	STRUCT_END
	ZERO_TAG
	SIMPLE_LIST
)

var typeToStr []string = []string{
	"Byte",
	"Short",
	"Int",
	"Long",
	"Float",
	"Double",
	"String1",
	"String4",
	"Map",
	"List",
	"StructBegin",
	"StructEnd",
	"ZeroTag",
	"SimpleList",
}

type Reader struct {
	ref []byte
	buf *bytes.Reader
}

// 提供兼容
func NewReader(data []byte) *Reader {
	return &Reader{buf: bytes.NewReader(data), ref: data}
}

// NewReader(FromInt8(vec))
func FromInt8(vec []int8) []byte {
	return *(*[]byte)(unsafe.Pointer(&vec))
}

//go:nosplit
func bReadU8(r *bytes.Reader, data *uint8) error {
	var err error
	*data, err = r.ReadByte()
	return err
}

//go:nosplit
func bReadU16(r *bytes.Reader, data *uint16) error {
	var b [2]byte
	var bs []byte
	bs = b[:]
	_, err := r.Read(bs)
	*data = binary.BigEndian.Uint16(bs)
	return err
}

//go:nosplit
func bReadU32(r *bytes.Reader, data *uint32) error {
	var b [4]byte
	var bs []byte
	bs = b[:]
	_, err := r.Read(bs)
	*data = binary.BigEndian.Uint32(bs)
	return err
}

//go:nosplit
func bReadU64(r *bytes.Reader, data *uint64) error {
	var b [8]byte
	var bs []byte
	bs = b[:]
	_, err := r.Read(bs)
	*data = binary.BigEndian.Uint64(bs)
	return err
}

//go:nosplit
func (b *Reader) readHead() (ty, tag byte, err error) {
	data, err := b.buf.ReadByte()
	if err != nil {
		return
	}
	ty = byte(data & 0x0f)
	tag = (data & 0xf0) >> 4
	if tag == 15 {
		data, err = b.buf.ReadByte()
		if err != nil {
			return
		}
		tag = data
	}
	return
}

func (b *Reader) unreadHead(tag byte) {
	b.buf.UnreadByte()
	if tag >= 15 {
		b.buf.UnreadByte()
	}
}

//go:nosplit
func (b *Reader) Next(n int) []byte {
	if n <= 0 {
		return []byte{}
	}
	beg := len(b.ref) - b.buf.Len()
	b.buf.Seek(int64(n), io.SeekCurrent)
	end := len(b.ref) - b.buf.Len()
	return b.ref[beg:end]
}

//go:nosplit
func (b *Reader) Skip(n int) {
	if n <= 0 {
		return
	}
	b.buf.Seek(int64(n), io.SeekCurrent)
}

func (b *Reader) skipField(ty byte) error {
	switch ty {
	case BYTE:
		b.Skip(1)
		break
	case SHORT:
		b.Skip(2)
		break
	case INT:
		b.Skip(4)
		break
	case LONG:
		b.Skip(8)
		break
	case FLOAT:
		b.Skip(4)
		break
	case DOUBLE:
		b.Skip(8)
		break
	case STRING1:
		data, err := b.buf.ReadByte()
		if err != nil {
			return err
		}
		l := int(data)
		b.Skip(l)
		break
	case STRING4:
		var l uint32
		err := bReadU32(b.buf, &l)
		if err != nil {
			return err
		}
		b.Skip(int(l))
		break
	case MAP:
		var len int32
		err := b.Read_int32(&len, 0, true)
		if err != nil {
			return err
		}

		for i := int32(0); i < len*2; i++ {
			tyCur, _, err := b.readHead()
			if err != nil {
				return err
			}
			b.skipField(tyCur)
		}
		break
	case LIST:
		var len int32
		err := b.Read_int32(&len, 0, true)
		if err != nil {
			return err
		}
		for i := int32(0); i < len; i++ {
			tyCur, _, err := b.readHead()
			if err != nil {
				return err
			}
			b.skipField(tyCur)
		}
		break
	case SIMPLE_LIST:
		tyCur, _, err := b.readHead()
		if tyCur != BYTE {
			return fmt.Errorf("simple list need byte head. but get %d", tyCur)
		}
		var len int32
		err = b.Read_int32(&len, 0, true)
		if err != nil {
			return err
		}

		b.Skip(int(len))
		break
	case STRUCT_BEGIN:
		err := b.SkipToStructEnd()
		if err != nil {
			return err
		}
		break
	case STRUCT_END:
		break
	case ZERO_TAG:
		break
	default:
		return fmt.Errorf("invalid type.")
	}
	return nil
}

// 用来跳转到对应层级STRUCT_END，支持内部嵌套其他STRUCT
func (b *Reader) SkipToStructEnd() error {
	for {
		ty, _, err := b.readHead()
		if err != nil {
			return err
		}

		err = b.skipField(ty)
		if err != nil {
			return err
		}
		if ty == STRUCT_END {
			break
		}
	}
	return nil
}

// 用来跳转到非STRUCT_END的其他tag，不用用于跳转STRUCT_END
func (b *Reader) SkipToNoCheck(tag byte, require bool) (error, bool, byte) {
	for {
		tyCur, tagCur, err := b.readHead()
		if err != nil {
			if require {
				return fmt.Errorf("Can not find Tag %d. But require. %s", tag, err.Error()),
					false, tyCur
			}
			return nil, false, tyCur
		}
		if tyCur == STRUCT_END || tagCur > tag {
			if require {
				return fmt.Errorf("Can not find Tag %d. But require. tagCur: %d, tyCur: %d",
					tag, tagCur, tyCur), false, tyCur
			}
			// 多读了一个head, 退回去.
			b.unreadHead(tagCur)
			return nil, false, tyCur
		}
		if tagCur == tag {
			return nil, true, tyCur
		}

		// tagCur < tag
		if err = b.skipField(tyCur); err != nil {
			return err, false, tyCur
		}
	}
}

func (b *Reader) SkipTo(ty, tag byte, require bool) (error, bool) {
	err, have, tyCur := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err, false
	}
	if have && ty != tyCur {
		return fmt.Errorf("type not match, need %d, bug %d.", ty, tyCur), false
	}
	return nil, have
}

func (b *Reader) Read_slice_int8(data *[]int8, len int32, require bool) error {
	if len <= 0 {
		return nil
	}
	*data = make([]int8, len)
	_, err := b.buf.Read(*(*[]uint8)(unsafe.Pointer(data)))
	if err != nil {
		err = fmt.Errorf("Read_slice_int8 error:%v", err)
	}
	return err
}

func (b *Reader) Read_slice_uint8(data *[]uint8, len int32, require bool) error {
	if len <= 0 {
		return nil
	}
	*data = make([]uint8, len)
	_, err := b.buf.Read(*data)
	if err != nil {
		err = fmt.Errorf("Read_slice_uint8 error:%v", err)
	}
	return err
}

func (b *Reader) Read_int8(data *int8, tag byte, require bool) error {
	err, have, ty := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	switch ty {
	case ZERO_TAG:
		*data = 0
	case BYTE:
		var tmp uint8
		err = bReadU8(b.buf, &tmp)
		*data = int8(tmp)
	default:
		return fmt.Errorf("read 'int8' type mismatch, tag:%d, get type:%s", tag, typeToStr[ty])
	}
	if err != nil {
		err = fmt.Errorf("Read_int8 tag:%d error:%v", tag, err)
	}
	return err
}

func (b *Reader) Read_uint8(data *uint8, tag byte, require bool) error {
	n := int16(*data)
	err := b.Read_int16(&n, tag, require)
	*data = uint8(n)
	return err
}

func (b *Reader) Read_bool(data *bool, tag byte, require bool) error {
	var tmp int8
	err := b.Read_int8(&tmp, tag, require)
	if err != nil {
		return err
	}
	if tmp == 0 {
		*data = false
	} else {
		*data = true
	}
	return nil
}

func (b *Reader) Read_int16(data *int16, tag byte, require bool) error {
	err, have, ty := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	switch ty {
	case ZERO_TAG:
		*data = 0
	case BYTE:
		var tmp uint8
		err = bReadU8(b.buf, &tmp)
		*data = int16(int8(tmp))
	case SHORT:
		var tmp uint16
		err = bReadU16(b.buf, &tmp)
		*data = int16(tmp)
	default:
		return fmt.Errorf("read 'int16' type mismatch, tag:%d, get type:%s", tag, typeToStr[ty])
	}
	if err != nil {
		err = fmt.Errorf("Read_int16 tag:%d error:%v", tag, err)
	}
	return err
}

func (b *Reader) Read_uint16(data *uint16, tag byte, require bool) error {
	n := int32(*data)
	err := b.Read_int32(&n, tag, require)
	*data = uint16(n)
	return err
}

func (b *Reader) Read_int32(data *int32, tag byte, require bool) error {
	err, have, ty := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	switch ty {
	case ZERO_TAG:
		*data = 0
	case BYTE:
		var tmp uint8
		err = bReadU8(b.buf, &tmp)
		*data = int32(int8(tmp))
	case SHORT:
		var tmp uint16
		err = bReadU16(b.buf, &tmp)
		*data = int32(int16(tmp))
	case INT:
		var tmp uint32
		err = bReadU32(b.buf, &tmp)
		*data = int32(tmp)
	default:
		return fmt.Errorf("read 'int32' type mismatch, tag:%d, get type:%s", tag, typeToStr[ty])
	}
	if err != nil {
		err = fmt.Errorf("Read_int32 tag:%d error:%v", tag, err)
	}
	return err
}

func (b *Reader) Read_uint32(data *uint32, tag byte, require bool) error {
	n := int64(*data)
	err := b.Read_int64(&n, tag, require)
	*data = uint32(n)
	return err
}

func (b *Reader) Read_int64(data *int64, tag byte, require bool) error {
	err, have, ty := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	switch ty {
	case ZERO_TAG:
		*data = 0
	case BYTE:
		var tmp uint8
		err = bReadU8(b.buf, &tmp)
		*data = int64(int8(tmp))
	case SHORT:
		var tmp uint16
		err = bReadU16(b.buf, &tmp)
		*data = int64(int16(tmp))
	case INT:
		var tmp uint32
		err = bReadU32(b.buf, &tmp)
		*data = int64(int32(tmp))
	case LONG:
		var tmp uint64
		err = bReadU64(b.buf, &tmp)
		*data = int64(tmp)
	default:
		return fmt.Errorf("read 'int64' type mismatch, tag:%d, get type:%s", tag, typeToStr[ty])
	}
	if err != nil {
		err = fmt.Errorf("Read_int64 tag:%d error:%v", tag, err)
	}

	return err
}

func (b *Reader) Read_float32(data *float32, tag byte, require bool) error {
	err, have := b.SkipTo(FLOAT, tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	var tmp uint32
	err = bReadU32(b.buf, &tmp)
	*data = math.Float32frombits(tmp)
	if err != nil {
		err = fmt.Errorf("Read_float32 tag:%d error:%v", tag, err)
	}
	return err
}

func (b *Reader) Read_float64(data *float64, tag byte, require bool) error {
	err, have := b.SkipTo(DOUBLE, tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}
	var tmp uint64
	err = bReadU64(b.buf, &tmp)
	*data = math.Float64frombits(tmp)
	if err != nil {
		err = fmt.Errorf("Read_float64 tag:%d error:%v", tag, err)
	}
	return err
}

func (b *Reader) Read_string(data *string, tag byte, require bool) error {
	err, have, ty := b.SkipToNoCheck(tag, require)
	if err != nil {
		return err
	}
	if !have {
		return nil
	}

	if ty == STRING4 {
		var len uint32
		err = bReadU32(b.buf, &len)
		if err != nil {
			return fmt.Errorf("Read_string4 tag:%d error:%v", tag, err)
		}
		buff := b.Next(int(len))
		*data = string(buff)
	} else if ty == STRING1 {
		var len uint8
		err = bReadU8(b.buf, &len)
		if err != nil {
			return fmt.Errorf("Read_string1 tag:%d error:%v", tag, err)
		}
		buff := b.Next(int(len))
		*data = string(buff)
	} else {
		return fmt.Errorf("need string, tag:%d, but type is %s.", tag, typeToStr[ty])
	}
	return nil
}
