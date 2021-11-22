// Package loadbalance 负载均衡组件，可插拔
package loadbalance

import (
	"errors"
	"sync"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// 负载均衡错误信息
var (
	ErrNoServerAvailable = errors.New("no server is available")
)

// 负载均衡策略
const (
	LoadBalanceRandom             = "random"
	LoadBalanceRoundRobin         = "round_robin"
	LoadBalanceWeightedRoundRobin = "weight_round_robin"
	LoadBalanceConsistentHash     = "consistent_hash"
)

// DefaultLoadBalancer 默认的负载均衡实现
var DefaultLoadBalancer LoadBalancer = &Random{}

// SetDefaultLoadBalancer 设置默认负载均衡
func SetDefaultLoadBalancer(b LoadBalancer) {
	DefaultLoadBalancer = b
}

// LoadBalancer 负载均衡接口，通过node数组返回一个node
type LoadBalancer interface {
	Select(serviceName string, list []*registry.Node, opt ...Option) (node *registry.Node, err error)
}

var (
	loadbalancers = make(map[string]LoadBalancer)
	lock          = sync.RWMutex{}
)

// Register 注册loadbalancer
func Register(name string, s LoadBalancer) {
	lock.Lock()
	loadbalancers[name] = s
	lock.Unlock()
}

// Get 获取loadbalancer
func Get(name string) LoadBalancer {
	lock.RLock()
	lb := loadbalancers[name]
	lock.RUnlock()
	return lb
}
