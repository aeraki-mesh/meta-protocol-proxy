package discovery

import (
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// IPDiscovery  ip列表服务发现
type IPDiscovery struct{}

// List 返回原始ipport
func (*IPDiscovery) List(serviceName string, opt ...Option) ([]*registry.Node, error) {
	node := &registry.Node{ServiceName: serviceName, Address: serviceName}

	return []*registry.Node{node}, nil
}
