// Package commhead comment
// This file war generated by trpc4videopacket 1.0
// Generated from VideoCommHead.jce
package commhead

import (
	"fmt"
	"git.code.oa.com/jce/jce"
)

// HAccessInfo struct implement
type HAccessInfo struct {
	ProxyIP           int32           `json:"ProxyIP"`
	ServerIP          int32           `json:"ServerIP"`
	ClientIP          int64           `json:"ClientIP"`
	ClientPort        int16           `json:"ClientPort"`
	ServiceTime       int32           `json:"ServiceTime"`
	ServiceName       string          `json:"ServiceName"`
	RtxName           string          `json:"RtxName"`
	FileName          string          `json:"FileName"`
	FuncName          string          `json:"FuncName"`
	Line              int32           `json:"Line"`
	CgiProcId         string          `json:"CgiProcId"`
	FromInfo          string          `json:"FromInfo"`
	AccIP             int64           `json:"AccIP"`
	AccPort           int32           `json:"AccPort"`
	AccId             int64           `json:"AccId"`
	ClientID          int64           `json:"ClientID"`
	QUAInfo           HQua            `json:"QUAInfo"`
	Guid              string          `json:"Guid"`
	BossReport        LogReport       `json:"BossReport"`
	Flag              int32           `json:"Flag"`
	Seq               int64           `json:"Seq"`
	ExtentAccountList []ExtentAccount `json:"extentAccountList"`
	AreaCodeInfo      string          `json:"AreaCodeInfo"`
	HttpCookie        string          `json:"HttpCookie"`
	Ipv6Addr          string          `json:"Ipv6Addr"`
	Ipv6tov4addr      string          `json:"Ipv6tov4addr"`
}

func (st *HAccessInfo) ResetDefault() {
}

// ReadFrom reads  from _is and put into struct.
func (st *HAccessInfo) ReadFrom(_is *jce.Reader) error {
	var err error
	var length int32
	var have bool
	var ty byte
	st.ResetDefault()

	err = _is.Read_int32(&st.ProxyIP, 0, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.ServerIP, 1, false)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.ClientIP, 2, false)
	if err != nil {
		return err
	}

	err = _is.Read_int16(&st.ClientPort, 3, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.ServiceTime, 4, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.ServiceName, 5, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.RtxName, 6, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.FileName, 7, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.FuncName, 8, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.Line, 9, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.CgiProcId, 10, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.FromInfo, 11, false)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.AccIP, 12, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.AccPort, 13, false)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.AccId, 14, false)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.ClientID, 15, false)
	if err != nil {
		return err
	}

	err = st.QUAInfo.ReadBlock(_is, 16, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Guid, 17, false)
	if err != nil {
		return err
	}

	err = st.BossReport.ReadBlock(_is, 18, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.Flag, 19, false)
	if err != nil {
		return err
	}

	err = _is.Read_int64(&st.Seq, 20, false)
	if err != nil {
		return err
	}

	err, have, ty = _is.SkipToNoCheck(21, false)
	if err != nil {
		return err
	}

	if have {
		if ty == jce.LIST {
			err = _is.Read_int32(&length, 0, true)
			if err != nil {
				return err
			}

			st.ExtentAccountList = make([]ExtentAccount, length, length)
			for i0, e0 := int32(0), length; i0 < e0; i0++ {

				err = st.ExtentAccountList[i0].ReadBlock(_is, 0, false)
				if err != nil {
					return err
				}

			}
		} else if ty == jce.SIMPLE_LIST {
			err = fmt.Errorf("not support simple_list type")
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

	err = _is.Read_string(&st.AreaCodeInfo, 22, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.HttpCookie, 23, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Ipv6Addr, 24, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Ipv6tov4addr, 25, false)
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
func (st *HAccessInfo) ReadBlock(_is *jce.Reader, tag byte, require bool) error {
	var err error
	var have bool
	st.ResetDefault()

	err, have = _is.SkipTo(jce.STRUCT_BEGIN, tag, require)
	if err != nil {
		return err
	}
	if !have {
		if require {
			return fmt.Errorf("require HAccessInfo, but not exist. tag %d", tag)
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
func (st *HAccessInfo) WriteTo(_os *jce.Buffer) error {
	var err error
	_ = err

	err = _os.Write_int32(st.ProxyIP, 0)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.ServerIP, 1)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.ClientIP, 2)
	if err != nil {
		return err
	}

	err = _os.Write_int16(st.ClientPort, 3)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.ServiceTime, 4)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.ServiceName, 5)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.RtxName, 6)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.FileName, 7)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.FuncName, 8)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.Line, 9)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.CgiProcId, 10)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.FromInfo, 11)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.AccIP, 12)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.AccPort, 13)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.AccId, 14)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.ClientID, 15)
	if err != nil {
		return err
	}

	err = st.QUAInfo.WriteBlock(_os, 16)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Guid, 17)
	if err != nil {
		return err
	}

	err = st.BossReport.WriteBlock(_os, 18)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.Flag, 19)
	if err != nil {
		return err
	}

	err = _os.Write_int64(st.Seq, 20)
	if err != nil {
		return err
	}

	err = _os.WriteHead(jce.LIST, 21)
	if err != nil {
		return err
	}

	err = _os.Write_int32(int32(len(st.ExtentAccountList)), 0)
	if err != nil {
		return err
	}

	for _, v := range st.ExtentAccountList {

		err = v.WriteBlock(_os, 0)
		if err != nil {
			return err
		}

	}

	err = _os.Write_string(st.AreaCodeInfo, 22)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.HttpCookie, 23)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Ipv6Addr, 24)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Ipv6tov4addr, 25)
	if err != nil {
		return err
	}

	return nil
}

//WriteBlock encode struct
func (st *HAccessInfo) WriteBlock(_os *jce.Buffer, tag byte) error {
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