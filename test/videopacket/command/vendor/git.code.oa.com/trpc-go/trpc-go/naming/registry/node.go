package registry

import (
	"fmt"
	"time"
)

// Node 服务节点信息
type Node struct {
	ServiceName   string        // 服务名
	ContainerName string        // 容器名
	Address       string        // 目标地址 ip:port
	Network       string        // 网络层协议 tcp/udp
	Protocol      string        // 业务协议 trpc/http
	Weight        int           // 权重
	CostTime      time.Duration // 当次请求耗时
	EnvKey        string        // 透传的环境信息
	Metadata      map[string]interface{}
}

// String node节点关键信息描述
func (n *Node) String() string {
	return fmt.Sprintf("service:%s, addr:%s, cost:%s", n.ServiceName, n.Address, n.CostTime)
}
