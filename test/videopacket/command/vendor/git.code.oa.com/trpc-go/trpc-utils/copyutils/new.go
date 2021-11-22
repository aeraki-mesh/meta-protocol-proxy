// Package copyutils provides some useful copy functions for slime.
package copyutils

import (
	"reflect"
)

// New create a new interface which has the same underlying type as v.
func New(v interface{}) interface{} {
	if v == nil {
		return nil
	}
	return reflect.New(
		reflect.Indirect(
			reflect.ValueOf(v),
		).Type(),
	).Interface()
}
