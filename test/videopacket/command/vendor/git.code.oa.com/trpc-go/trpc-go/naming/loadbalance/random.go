package loadbalance

import (
	"math/rand"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

func init() {
	rand.Seed(time.Now().Unix())

	Register(LoadBalanceRandom, &Random{})
}

// Random 随机负载均衡
type Random struct{}

// Select 随机从列表挑选一个节点
func (b *Random) Select(serviceName string, list []*registry.Node, opt ...Option) (
	*registry.Node, error) {

	if len(list) == 0 {
		return nil, ErrNoServerAvailable
	}

	n := rand.Intn(len(list))
	return list[n], nil
}
