package selector

import (
	"errors"
	"math/rand"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/naming/circuitbreaker"
	"git.code.oa.com/trpc-go/trpc-go/naming/discovery"
	"git.code.oa.com/trpc-go/trpc-go/naming/loadbalance"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
	"git.code.oa.com/trpc-go/trpc-go/naming/servicerouter"
)

func init() {
	rand.Seed(time.Now().Unix())
}

// 路由选择失败信息
var (
	ErrReportNodeEmpty             = errors.New("selector report node empty")
	ErrReportMetaDataEmpty         = errors.New("selector report metadata empty")
	ErrReportNoCircuitBreaker      = errors.New("selector report not circuitbreaker")
	ErrReportInvalidCircuitBreaker = errors.New("selector report circuitbreaker invalid")
)

// DefaultSelector 默认的选择器
var DefaultSelector Selector = &TrpcSelector{}

// TrpcSelector trpc默认实现，内部自动串好 服务发现 负载均衡 熔断隔离 等流程，每个子组件都可插拔
type TrpcSelector struct{}

// Select 输入service name,返回一个可用的node
func (s *TrpcSelector) Select(serviceName string, opt ...Option) (*registry.Node, error) {

	if serviceName == "" {
		return nil, errors.New("service name empty")
	}

	opts := &Options{
		Discovery:      discovery.DefaultDiscovery,
		LoadBalance:    loadbalance.DefaultLoadBalancer,
		CircuitBreaker: circuitbreaker.DefaultCircuitBreaker,
		ServiceRouter:  servicerouter.DefaultServiceRouter,
	}
	for _, o := range opt {
		o(opts)
	}

	if opts.Discovery == nil {
		return nil, errors.New("discovery not exists")
	}
	list, err := opts.Discovery.List(serviceName, opts.DiscoveryOptions...)
	if err != nil {
		return nil, err
	}

	if opts.ServiceRouter == nil {
		return nil, errors.New("servicerouter not exists")
	}
	list, err = opts.ServiceRouter.Filter(serviceName, list, opts.ServiceRouterOptions...)
	if err != nil {
		return nil, err
	}

	if opts.LoadBalance == nil {
		return nil, errors.New("loadbalance not exists")
	}

	if opts.CircuitBreaker == nil {
		return nil, errors.New("circuitbreaker not exists")
	}

	node, err := opts.LoadBalance.Select(serviceName, list, opts.LoadBalanceOptions...)
	if err != nil {
		return nil, err
	}

	if len(node.Metadata) == 0 {
		node.Metadata = make(map[string]interface{})
	}
	node.Metadata["circuitbreaker"] = opts.CircuitBreaker
	return node, nil
}

// Report 上报结果
func (s *TrpcSelector) Report(node *registry.Node, cost time.Duration, err error) error {
	if node == nil {
		return ErrReportNodeEmpty
	}
	if node.Metadata == nil {
		return ErrReportMetaDataEmpty
	}
	breaker, ok := node.Metadata["circuitbreaker"]
	if !ok {
		return ErrReportNoCircuitBreaker
	}
	circuitbreaker, ok := breaker.(circuitbreaker.CircuitBreaker)
	if !ok {
		return ErrReportInvalidCircuitBreaker
	}
	return circuitbreaker.Report(node, cost, err)
}
