package copyutils

import (
	"errors"
	"fmt"
	"reflect"
)

// ShallowCopy copies src to dst with only one copy depth.
//
// The native CopyTo method will be used if src implements one.
//
// ShallowCopy does copy unexported fields.
// dst must be a pointer type.
func ShallowCopy(dst, src interface{}) (err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("shallow copy should never panic: %v", r)
		}
	}()

	if src == nil {
		return nil
	}

	if copierTo, ok := src.(CopierTo); ok {
		return copierTo.CopyTo(dst)
	}

	dstV := reflect.ValueOf(dst)
	if dstV.Kind() != reflect.Ptr {
		return errors.New("dst must be a pointer")
	}

	dstV = reflect.Indirect(dstV)
	srcV := reflect.Indirect(reflect.ValueOf(src))

	if dstV.Type() != srcV.Type() {
		return fmt.Errorf("dst %s and src %s type miss match", dstV.Type(), srcV.Type())
	}

	dstV.Set(srcV)

	return nil
}
