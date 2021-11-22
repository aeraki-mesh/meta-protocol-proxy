// Package commhead comment
// This file war generated by trpc4videopacket 1.0
// Generated from VideoCommHead.jce
package commhead

import (
	"fmt"
	"git.code.oa.com/jce/jce"
)

// ServerRoute struct implement
type ServerRoute struct {
	FromServerName  string `json:"fromServerName"`
	FromServerSetId int32  `json:"fromServerSetId"`
	ToServerName    string `json:"toServerName"`
	ToServerSetId   int32  `json:"toServerSetId"`
	FuncName        string `json:"FuncName"`
	FromFuncName    string `json:"fromFuncName"`
}

func (st *ServerRoute) ResetDefault() {
}

// ReadFrom reads  from _is and put into struct.
func (st *ServerRoute) ReadFrom(_is *jce.Reader) error {
	var err error
	var length int32
	var have bool
	var ty byte
	st.ResetDefault()

	err = _is.Read_string(&st.FromServerName, 0, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.FromServerSetId, 1, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.ToServerName, 2, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.ToServerSetId, 3, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.FuncName, 4, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.FromFuncName, 5, false)
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
func (st *ServerRoute) ReadBlock(_is *jce.Reader, tag byte, require bool) error {
	var err error
	var have bool
	st.ResetDefault()

	err, have = _is.SkipTo(jce.STRUCT_BEGIN, tag, require)
	if err != nil {
		return err
	}
	if !have {
		if require {
			return fmt.Errorf("require ServerRoute, but not exist. tag %d", tag)
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
func (st *ServerRoute) WriteTo(_os *jce.Buffer) error {
	var err error
	_ = err

	err = _os.Write_string(st.FromServerName, 0)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.FromServerSetId, 1)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.ToServerName, 2)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.ToServerSetId, 3)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.FuncName, 4)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.FromFuncName, 5)
	if err != nil {
		return err
	}

	return nil
}

//WriteBlock encode struct
func (st *ServerRoute) WriteBlock(_os *jce.Buffer, tag byte) error {
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
