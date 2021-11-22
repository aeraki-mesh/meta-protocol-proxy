package objectpool

import (
	"bytes"
	"sync"
)

// BufferPool buffer对象池
type BufferPool struct {
	pool sync.Pool
}

// NewBufferPool 新建bytes.Buffer对象池
func NewBufferPool() *BufferPool {
	return &BufferPool{
		pool: sync.Pool{
			New: func() interface{} {
				return new(bytes.Buffer)
			},
		},
	}
}

// Get 从池子拿出buffer
func (p *BufferPool) Get() *bytes.Buffer {
	return p.pool.Get().(*bytes.Buffer)
}

// Put buffer放回池子
func (p *BufferPool) Put(buf *bytes.Buffer) {
	p.pool.Put(buf)
}
