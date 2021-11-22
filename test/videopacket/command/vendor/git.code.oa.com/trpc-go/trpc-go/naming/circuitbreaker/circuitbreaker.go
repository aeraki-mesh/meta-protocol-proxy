// Package circuitbreaker 熔断器组件，可插拔
package circuitbreaker

import (
	"sync"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// DefaultCircuitBreaker 默认的熔断器
var DefaultCircuitBreaker CircuitBreaker = &NoopCircuitBreaker{}

// SetDefaultCircuitBreaker 设置默认熔断器
func SetDefaultCircuitBreaker(cb CircuitBreaker) {
	DefaultCircuitBreaker = cb
}

// CircuitBreaker 熔断器接口，判断node是否可用，上报当前node成功或者失败
type CircuitBreaker interface {
	Available(node *registry.Node) bool
	Report(node *registry.Node, cost time.Duration, err error) error
}

var (
	circuitbreakers = make(map[string]CircuitBreaker)
	lock            = sync.RWMutex{}
)

// Register 注册circuitbreaker
func Register(name string, s CircuitBreaker) {
	lock.Lock()
	circuitbreakers[name] = s
	lock.Unlock()
}

// Get 获取circuitbreaker
func Get(name string) CircuitBreaker {
	lock.RLock()
	c := circuitbreakers[name]
	lock.RUnlock()
	return c
}

func unregisterForTesting(name string) {
	lock.Lock()
	delete(circuitbreakers, name)
	lock.Unlock()
}

// NoopCircuitBreaker 空熔断器
type NoopCircuitBreaker struct{}

// Available 空熔断器，不熔断，永远为true可用
func (*NoopCircuitBreaker) Available(*registry.Node) bool {
	return true
}

// Report 空熔断器 不上报
func (*NoopCircuitBreaker) Report(*registry.Node, time.Duration, error) error {
	return nil
}
