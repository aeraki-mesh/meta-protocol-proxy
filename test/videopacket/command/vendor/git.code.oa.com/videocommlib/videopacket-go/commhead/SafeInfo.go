// Package commhead comment
// This file war generated by trpc4videopacket 1.0
// Generated from VideoCommHead.jce
package commhead

import (
	"fmt"
	"git.code.oa.com/jce/jce"
)

// SafeInfo struct implement
type SafeInfo struct {
	Type       int32  `json:"type"`
	SafeKey    string `json:"SafeKey"`
	SafeResult []int8 `json:"SafeResult"`
}

func (st *SafeInfo) ResetDefault() {
}

// ReadFrom reads  from _is and put into struct.
func (st *SafeInfo) ReadFrom(_is *jce.Reader) error {
	var err error
	var length int32
	var have bool
	var ty byte
	st.ResetDefault()

	err = _is.Read_int32(&st.Type, 0, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.SafeKey, 1, false)
	if err != nil {
		return err
	}

	err, have, ty = _is.SkipToNoCheck(2, false)
	if err != nil {
		return err
	}

	if have {
		if ty == jce.LIST {
			err = _is.Read_int32(&length, 0, true)
			if err != nil {
				return err
			}

			st.SafeResult = make([]int8, length, length)
			for i0, e0 := int32(0), length; i0 < e0; i0++ {

				err = _is.Read_int8(&st.SafeResult[i0], 0, false)
				if err != nil {
					return err
				}

			}
		} else if ty == jce.SIMPLE_LIST {

			err, _ = _is.SkipTo(jce.BYTE, 0, true)
			if err != nil {
				return err
			}

			err = _is.Read_int32(&length, 0, true)
			if err != nil {
				return err
			}

			err = _is.Read_slice_int8(&st.SafeResult, length, true)
			if err != nil {
				return err
			}

		} else {
			err = fmt.Errorf("require vector, but not")
			if err != nil {
				return err
			}

		}
	}

	_ = err
	_ = length
	_ = have
	_ = ty
	return nil
}

//ReadBlock reads struct from the given tag , require or optional.
func (st *SafeInfo) ReadBlock(_is *jce.Reader, tag byte, require bool) error {
	var err error
	var have bool
	st.ResetDefault()

	err, have = _is.SkipTo(jce.STRUCT_BEGIN, tag, require)
	if err != nil {
		return err
	}
	if !have {
		if require {
			return fmt.Errorf("require SafeInfo, but not exist. tag %d", tag)
		}
		return nil

	}

	st.ReadFrom(_is)

	err = _is.SkipToStructEnd()
	if err != nil {
		return err
	}
	_ = have
	return nil
}

//WriteTo encode struct to buffer
func (st *SafeInfo) WriteTo(_os *jce.Buffer) error {
	var err error
	_ = err

	err = _os.Write_int32(st.Type, 0)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.SafeKey, 1)
	if err != nil {
		return err
	}

	err = _os.WriteHead(jce.SIMPLE_LIST, 2)
	if err != nil {
		return err
	}

	err = _os.WriteHead(jce.BYTE, 0)
	if err != nil {
		return err
	}

	err = _os.Write_int32(int32(len(st.SafeResult)), 0)
	if err != nil {
		return err
	}

	err = _os.Write_slice_int8(st.SafeResult)
	if err != nil {
		return err
	}

	return nil
}

//WriteBlock encode struct
func (st *SafeInfo) WriteBlock(_os *jce.Buffer, tag byte) error {
	var err error
	err = _os.WriteHead(jce.STRUCT_BEGIN, tag)
	if err != nil {
		return err
	}

	st.WriteTo(_os)

	err = _os.WriteHead(jce.STRUCT_END, 0)
	if err != nil {
		return err
	}
	return nil
}
