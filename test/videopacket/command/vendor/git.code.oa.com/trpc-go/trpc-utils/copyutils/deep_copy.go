package copyutils

import (
	"reflect"
)

// DeepCopy deeply copy v.
//
// The native Copy method will be used if v implements one.
//
// Pointer loops are preserved in new value.
// Slices sharing the same underlying array do not share in new value.
// Channels are directly copied.
// Unexported fields are not copied.
func DeepCopy(v interface{}) (interface{}, error) {
	if v == nil {
		return nil, nil
	}

	if copier, ok := v.(Copier); ok {
		return copier.Copy()
	}

	return copy(reflect.ValueOf(v), make(map[uintptr]reflect.Value)).Interface(), nil
}

func copy(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	switch v.Kind() {
	case reflect.Array:
		return copyArray(v, visited)
	case reflect.Slice:
		return copySlice(v, visited)
	case reflect.Map:
		return copyMap(v, visited)
	case reflect.Ptr:
		return copyPointer(v, visited)
	case reflect.Struct:
		return copyStruct(v, visited)
	case reflect.Interface:
		return copyInterface(v, visited)
	default:
		return copyDefault(v, visited)
	}
}

func copyArray(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	l, t := v.Len(), v.Type()
	vv := reflect.New(t).Elem()
	for i := 0; i < l; i++ {
		vv.Index(i).Set(copy(v.Index(i), visited))
	}

	return vv
}

func copySlice(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	if v.IsNil() {
		return v
	}

	l, t := v.Len(), v.Type()
	vv := reflect.MakeSlice(t, l, l)
	for i := 0; i < l; i++ {
		vv.Index(i).Set(copy(v.Index(i), visited))
	}

	return vv
}

func copyMap(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	if v.IsNil() {
		return v
	}

	if vv, ok := visited[v.Pointer()]; ok {
		return vv
	}

	vv := reflect.MakeMap(v.Type())
	visited[v.Pointer()] = vv

	iter := v.MapRange()
	for iter.Next() {
		vv.SetMapIndex(
			copy(iter.Key(), visited),
			copy(iter.Value(), visited))
	}

	return vv
}

func copyPointer(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	if v.IsNil() {
		return v
	}

	if vv, ok := visited[v.Pointer()]; ok {
		return vv
	}

	vv := reflect.New(v.Type().Elem())
	visited[v.Pointer()] = vv
	vv.Elem().Set(copy(v.Elem(), visited))

	return vv
}

func copyStruct(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	t := v.Type()
	vv := reflect.New(t)

	for i := 0; i < t.NumField(); i++ {
		f := t.Field(i)
		if f.PkgPath != "" {
			continue
		}
		vv.Elem().Field(i).Set(copy(v.Field(i), visited))
	}

	return vv.Elem()
}

func copyInterface(v reflect.Value, visited map[uintptr]reflect.Value) reflect.Value {
	if v.IsNil() {
		return v
	}

	vv := reflect.New(v.Type()).Elem()
	vv.Set(copy(v.Elem(), visited))

	return vv
}

func copyDefault(v reflect.Value, _ map[uintptr]reflect.Value) reflect.Value {
	return v
}
