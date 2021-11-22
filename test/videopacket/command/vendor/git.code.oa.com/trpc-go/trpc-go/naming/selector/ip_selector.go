// Package selector client后端路由选择器，通过service name获取一个节点，内部调用服务发现，负载均衡，熔断隔离
package selector

import (
	"errors"
	"math/rand"
	"strings"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

func init() {
	Register("ip", &ipSelector{})  // ip://ip:port
	Register("dns", &ipSelector{}) // dns://domain:port
}

type ipSelector struct {
}

// Select 默认的ip selector， 输入service name是 ip1:port1,ip2:port2, 支持多ip
func (s *ipSelector) Select(serviceName string, opt ...Option) (*registry.Node, error) {
	if serviceName == "" {
		return nil, errors.New("serviceName empty")
	}

	addrList := strings.Split(serviceName, ",")
	if len(addrList) == 1 {
		return &registry.Node{ServiceName: serviceName, Address: addrList[0]}, nil
	}

	addr := addrList[rand.Int()%len(addrList)]
	return &registry.Node{ServiceName: serviceName, Address: addr}, nil
}

// Report 空实现
func (s *ipSelector) Report(*registry.Node, time.Duration, error) error {
	return nil
}
