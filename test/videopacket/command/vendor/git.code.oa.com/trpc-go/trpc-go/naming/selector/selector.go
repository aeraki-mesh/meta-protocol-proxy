// Package selector client后端路由选择器，通过service name获取一个节点，内部调用服务发现，负载均衡，熔断隔离
package selector

import (
	"time"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// Selector 路由组件接口
type Selector interface {
	// Select 通过service name获取一个后端节点
	Select(serviceName string, opt ...Option) (*registry.Node, error)
	// Report 上报当前请求成功或失败
	Report(node *registry.Node, cost time.Duration, err error) error
}

var (
	selectors = make(map[string]Selector)
)

// Register 注册selector，如l5 dns cmlb tseer
func Register(name string, s Selector) {
	selectors[name] = s
}

// Get 获取selector
func Get(name string) Selector {
	s := selectors[name]
	return s
}

func unregisterForTesting(name string) {
	delete(selectors, name)
}
