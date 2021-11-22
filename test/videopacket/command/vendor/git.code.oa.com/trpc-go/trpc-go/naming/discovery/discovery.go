// Package discovery 服务发现组件，可插拔
package discovery

import (
	"sync"

	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
)

// DefaultDiscovery 默认的服务发现，由配置文件指定
var DefaultDiscovery Discovery = &IPDiscovery{}

// SetDefaultDiscovery 设置默认服务发现
func SetDefaultDiscovery(d Discovery) {
	DefaultDiscovery = d
}

// Discovery 服务发现接口，通过service name返回node数组
type Discovery interface {
	List(serviceName string, opt ...Option) (nodes []*registry.Node, err error)
}

var (
	discoveries = make(map[string]Discovery)
	lock        = sync.RWMutex{}
)

// Register 注册discovery
func Register(name string, s Discovery) {
	lock.Lock()
	discoveries[name] = s
	lock.Unlock()
}

// Get 获取discovery
func Get(name string) Discovery {
	lock.RLock()
	d := discoveries[name]
	lock.RUnlock()
	return d
}

func unregisterForTesting(name string) {
	lock.Lock()
	delete(discoveries, name)
	lock.Unlock()
}
