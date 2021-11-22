// Package registry 服务注册，server启动时 上报服务自身信息
package registry

import (
	"errors"
	"sync"
)

// ErrNotImplement 没有实现
var ErrNotImplement = errors.New("not implement")

// DefaultRegistry 默认的服务注册实现
var DefaultRegistry Registry = &NoopRegistry{}

// SetDefaultRegistry 设置默认服务注册实现
func SetDefaultRegistry(r Registry) {
	DefaultRegistry = r
}

// Registry 服务注册接口
type Registry interface {
	Register(service string, opt ...Option) error
	Deregister(service string) error
}

var (
	registries = make(map[string]Registry)
	lock       = sync.RWMutex{}
)

// Register 通过service name注册registry,一个service一个registry
func Register(name string, s Registry) {
	lock.Lock()
	registries[name] = s
	lock.Unlock()
}

// Get 获取registry
func Get(name string) Registry {
	lock.RLock()
	r := registries[name]
	lock.RUnlock()
	return r
}

func unregisterForTesting(name string) {
	lock.Lock()
	delete(registries, name)
	lock.Unlock()
}

// NoopRegistry 空注册
type NoopRegistry struct{}

// Register 返回失败
func (noop *NoopRegistry) Register(service string, opt ...Option) error {
	return ErrNotImplement
}

// Deregister 返回失败
func (noop *NoopRegistry) Deregister(service string) error {
	return ErrNotImplement
}
