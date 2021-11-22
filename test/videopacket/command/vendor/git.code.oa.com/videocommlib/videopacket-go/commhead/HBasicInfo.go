// Package commhead comment
// This file war generated by trpc4videopacket 1.0
// Generated from VideoCommHead.jce
package commhead

import (
	"fmt"
	"git.code.oa.com/jce/jce"
)

// HBasicInfo struct implement
type HBasicInfo struct {
	ReqUin      int64 `json:"ReqUin"`
	Command     int16 `json:"Command"`
	ServiceType int8  `json:"ServiceType"`
	Version     int8  `json:"version"`
	Result      int32 `json:"Result"`
	CallerID    int32 `json:"CallerID"`
	SeqId       int64 `json:"SeqId"`
	SubCmd      int32 `json:"SubCmd"`
	BodyFlag    int32 `json:"BodyFlag"`
}

func (st *HBasicInfo) ResetDefault() {
}

// ReadFrom reads  from _is and put into struct.
func (st *HBasicInfo) ReadFrom(_is *jce.Reader) error {
	var err error
	var length int32
	var have bool
	var ty byte
	st.ResetDefault()

	err = _is.Read_int64(&st.ReqUin, 0, true)
	if err != nil {
		return err
	}

	err = _is.Read_int16(&st.Command, 1, true)
	if err != nil {
		return err
	}

	err = _is.Read_int8(&st.ServiceType, 2, true)
	if err != nil {
		return err
	}

	err = _is.Read_int8(&st.Version, 3, true)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.Result, 4, true)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.CallerID, 5, true)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.SeqId, 6, true)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.SubCmd, 7, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.BodyFlag, 8, false)
	if err != nil {
		return err
	}

	_ = err
	_ = length
	_ = have
	_ = ty
	return nil
}

//ReadBlock reads struct from the given tag , require or optional.
func (st *HBasicInfo) ReadBlock(_is *jce.Reader, tag byte, require bool) error {
	var err error
	var have bool
	st.ResetDefault()

	err, have = _is.SkipTo(jce.STRUCT_BEGIN, tag, require)
	if err != nil {
		return err
	}
	if !have {
		if require {
			return fmt.Errorf("require HBasicInfo, but not exist. tag %d", tag)
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
func (st *HBasicInfo) WriteTo(_os *jce.Buffer) error {
	var err error
	_ = err

	err = _os.Write_int64(st.ReqUin, 0)
	if err != nil {
		return err
	}

	err = _os.Write_int16(st.Command, 1)
	if err != nil {
		return err
	}

	err = _os.Write_int8(st.ServiceType, 2)
	if err != nil {
		return err
	}

	err = _os.Write_int8(st.Version, 3)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.Result, 4)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.CallerID, 5)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.SeqId, 6)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.SubCmd, 7)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.BodyFlag, 8)
	if err != nil {
		return err
	}

	return nil
}

//WriteBlock encode struct
func (st *HBasicInfo) WriteBlock(_os *jce.Buffer, tag byte) error {
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
