// Package commhead comment
// This file war generated by trpc4videopacket 1.0
// Generated from VideoCommHead.jce
package commhead

import (
	"fmt"
	"git.code.oa.com/jce/jce"
)

// HttpRequest struct implement
type HttpRequest struct {
	ClientIP    int64             `json:"ClientIP"`
	RequestType int32             `json:"RequestType"`
	Url         string            `json:"Url"`
	Accept      string            `json:"Accept"`
	UserAgent   string            `json:"UserAgent"`
	Host        string            `json:"Host"`
	Referer     string            `json:"Referer"`
	PostData    string            `json:"PostData"`
	Cookies     map[string]string `json:"Cookies"`
	Queries     map[string]string `json:"Queries"`
	MoreHeaders map[string]string `json:"MoreHeaders"`
}

func (st *HttpRequest) ResetDefault() {
}

// ReadFrom reads  from _is and put into struct.
func (st *HttpRequest) ReadFrom(_is *jce.Reader) error {
	var err error
	var length int32
	var have bool
	var ty byte
	st.ResetDefault()

	err = _is.Read_int64(&st.ClientIP, 0, false)
	if err != nil {
		return err
	}

	err = _is.Read_int32(&st.RequestType, 1, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Url, 2, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Accept, 3, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.UserAgent, 4, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Host, 5, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.Referer, 6, false)
	if err != nil {
		return err
	}

	err = _is.Read_string(&st.PostData, 7, false)
	if err != nil {
		return err
	}

	err, have = _is.SkipTo(jce.MAP, 8, false)
	if err != nil {
		return err
	}

	if have {
		err = _is.Read_int32(&length, 0, true)
		if err != nil {
			return err
		}

		st.Cookies = make(map[string]string)
		for i0, e0 := int32(0), length; i0 < e0; i0++ {
			var k0 string
			var v0 string

			err = _is.Read_string(&k0, 0, false)
			if err != nil {
				return err
			}

			err = _is.Read_string(&v0, 1, false)
			if err != nil {
				return err
			}

			st.Cookies[k0] = v0
		}
	}

	err, have = _is.SkipTo(jce.MAP, 9, false)
	if err != nil {
		return err
	}

	if have {
		err = _is.Read_int32(&length, 0, true)
		if err != nil {
			return err
		}

		st.Queries = make(map[string]string)
		for i1, e1 := int32(0), length; i1 < e1; i1++ {
			var k1 string
			var v1 string

			err = _is.Read_string(&k1, 0, false)
			if err != nil {
				return err
			}

			err = _is.Read_string(&v1, 1, false)
			if err != nil {
				return err
			}

			st.Queries[k1] = v1
		}
	}

	err, have = _is.SkipTo(jce.MAP, 10, false)
	if err != nil {
		return err
	}

	if have {
		err = _is.Read_int32(&length, 0, true)
		if err != nil {
			return err
		}

		st.MoreHeaders = make(map[string]string)
		for i2, e2 := int32(0), length; i2 < e2; i2++ {
			var k2 string
			var v2 string

			err = _is.Read_string(&k2, 0, false)
			if err != nil {
				return err
			}

			err = _is.Read_string(&v2, 1, false)
			if err != nil {
				return err
			}

			st.MoreHeaders[k2] = v2
		}
	}

	_ = err
	_ = length
	_ = have
	_ = ty
	return nil
}

//ReadBlock reads struct from the given tag , require or optional.
func (st *HttpRequest) ReadBlock(_is *jce.Reader, tag byte, require bool) error {
	var err error
	var have bool
	st.ResetDefault()

	err, have = _is.SkipTo(jce.STRUCT_BEGIN, tag, require)
	if err != nil {
		return err
	}
	if !have {
		if require {
			return fmt.Errorf("require HttpRequest, but not exist. tag %d", tag)
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
func (st *HttpRequest) WriteTo(_os *jce.Buffer) error {
	var err error
	_ = err

	err = _os.Write_int64(st.ClientIP, 0)
	if err != nil {
		return err
	}

	err = _os.Write_int32(st.RequestType, 1)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Url, 2)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Accept, 3)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.UserAgent, 4)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Host, 5)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.Referer, 6)
	if err != nil {
		return err
	}

	err = _os.Write_string(st.PostData, 7)
	if err != nil {
		return err
	}

	err = _os.WriteHead(jce.MAP, 8)
	if err != nil {
		return err
	}

	err = _os.Write_int32(int32(len(st.Cookies)), 0)
	if err != nil {
		return err
	}

	for k3, v3 := range st.Cookies {

		err = _os.Write_string(k3, 0)
		if err != nil {
			return err
		}

		err = _os.Write_string(v3, 1)
		if err != nil {
			return err
		}

	}

	err = _os.WriteHead(jce.MAP, 9)
	if err != nil {
		return err
	}

	err = _os.Write_int32(int32(len(st.Queries)), 0)
	if err != nil {
		return err
	}

	for k4, v4 := range st.Queries {

		err = _os.Write_string(k4, 0)
		if err != nil {
			return err
		}

		err = _os.Write_string(v4, 1)
		if err != nil {
			return err
		}

	}

	err = _os.WriteHead(jce.MAP, 10)
	if err != nil {
		return err
	}

	err = _os.Write_int32(int32(len(st.MoreHeaders)), 0)
	if err != nil {
		return err
	}

	for k5, v5 := range st.MoreHeaders {

		err = _os.Write_string(k5, 0)
		if err != nil {
			return err
		}

		err = _os.Write_string(v5, 1)
		if err != nil {
			return err
		}

	}

	return nil
}

//WriteBlock encode struct
func (st *HttpRequest) WriteBlock(_os *jce.Buffer, tag byte) error {
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