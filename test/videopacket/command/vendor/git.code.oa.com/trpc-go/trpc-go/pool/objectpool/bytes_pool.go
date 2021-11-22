// Package objectpool 对象池
package objectpool

import (
	"sync"
)

// BytesPool bytes数组对象池
type BytesPool struct {
	pool sync.Pool
}

// NewBytesPool 新建bytes数组对象池
func NewBytesPool(size int) *BytesPool {
	return &BytesPool{
		pool: sync.Pool{
			New: func() interface{} {
				return make([]byte, size)
			},
		},
	}
}

// Get 从对象池拿出bytes数组
func (p *BytesPool) Get() []byte {
	return p.pool.Get().([]byte)
}

// Put bytes数组放回对象池
func (p *BytesPool) Put(b []byte) {
	p.pool.Put(b)
}
