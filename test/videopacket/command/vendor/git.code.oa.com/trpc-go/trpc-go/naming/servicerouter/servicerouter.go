// Package servicerouter 服务路由，处于服务发现和负载均衡之间，按条件过滤服务实例
package servicerouter

import (
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// DefaultServiceRouter 默认的服务路由，由配置文件指定
var DefaultServiceRouter ServiceRouter = &NoopServiceRouter{}

// SetDefaultServiceRouter 设置默认服务路由
func SetDefaultServiceRouter(s ServiceRouter) {
	DefaultServiceRouter = s
}

// ServiceRouter 服务路由接口
type ServiceRouter interface {
	Filter(serviceName string, nodes []*registry.Node, opt ...Option) ([]*registry.Node, error)
}

var (
	servicerouters = make(map[string]ServiceRouter)
)

// Register 注册 ServiceRouter
func Register(name string, s ServiceRouter) {
	servicerouters[name] = s
}

// Get 获取 ServiceRouter
func Get(name string) ServiceRouter {
	return servicerouters[name]
}

// NoopServiceRouter 空服务路由实现
type NoopServiceRouter struct {
}

// Filter 服务路由过滤过滤
func (*NoopServiceRouter) Filter(serviceName string, nodes []*registry.Node, opt ...Option) ([]*registry.Node, error) {
	return nodes, nil
}

func unregisterForTesting(name string) {
	delete(servicerouters, name)
}
