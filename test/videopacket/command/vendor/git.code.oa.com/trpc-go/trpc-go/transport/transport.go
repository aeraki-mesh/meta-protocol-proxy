// Package transport 底层网络通讯层，只负责最基本的二进制数据网络通信，没有任何业务逻辑，
// 默认全局只会有一个ServerTransport和一个ClientTransport，提供默认式可插拔能力。
package transport

import (
	"context"
	"net"
	"sync"

	"git.code.oa.com/trpc-go/trpc-go/codec"
)

var (
	svrTrans    = make(map[string]ServerTransport)
	muxSvrTrans = sync.RWMutex{}

	clientTrans    = make(map[string]ClientTransport)
	muxClientTrans = sync.RWMutex{}
)

// FramerBuilder alias codec.FramerBuilder
type FramerBuilder = codec.FramerBuilder

// Framer alias codec.Framer
type Framer = codec.Framer

type contextKey struct {
	name string
}

var (
	// LocalAddrContextKey 代表本机地址
	LocalAddrContextKey = &contextKey{"local-addr"}

	// RemoteAddrContextKey 代表远端地址
	RemoteAddrContextKey = &contextKey{"remote-addr"}
)

// RemoteAddrFromContext 从context中获取RemoteAddr
func RemoteAddrFromContext(ctx context.Context) net.Addr {
	addr, ok := ctx.Value(RemoteAddrContextKey).(net.Addr)
	if !ok {
		return nil
	}
	return addr
}

// ServerTransport server通讯层接口
type ServerTransport interface {
	ListenAndServe(ctx context.Context, opts ...ListenServeOption) error
}

// ClientTransport client通讯层接口
type ClientTransport interface {
	RoundTrip(ctx context.Context, req []byte, opts ...RoundTripOption) (rsp []byte, err error)
}

// Handler server transport收到包后的处理函数
type Handler interface {
	Handle(ctx context.Context, req []byte) (rsp []byte, err error)
}

var framerBuilders = make(map[string]codec.FramerBuilder)

// RegisterFramerBuilder 注册FramerBuilder
func RegisterFramerBuilder(name string, fb codec.FramerBuilder) {

	if fb == nil {
		panic("transport: register framerBuilders nil")
	}
	if name == "" {
		panic("transport: register framerBuilders name empty")
	}
	framerBuilders[name] = fb
}

// RegisterServerTransport 注册server transport
func RegisterServerTransport(name string, t ServerTransport) {
	if t == nil {
		panic("transport: register nil server transport")
	}
	if name == "" {
		panic("transport: register empty name of server transport")
	}
	muxSvrTrans.Lock()
	svrTrans[name] = t
	muxSvrTrans.Unlock()
}

// GetServerTransport 获取server transport
func GetServerTransport(name string) ServerTransport {
	muxSvrTrans.RLock()
	t := svrTrans[name]
	muxSvrTrans.RUnlock()
	return t
}

// RegisterClientTransport 注册client端的transport插件
func RegisterClientTransport(name string, t ClientTransport) {
	if t == nil {
		panic("transport: register nil client transport")
	}
	if name == "" {
		panic("transport: register empty name of client transport")
	}
	muxClientTrans.Lock()
	clientTrans[name] = t
	muxClientTrans.Unlock()
}

// GetClientTransport 获取client端的transport插件
func GetClientTransport(name string) ClientTransport {
	muxClientTrans.RLock()
	t := clientTrans[name]
	muxClientTrans.RUnlock()
	return t
}

// GetFramerBuilder 根据名字获取特定的FramerBuilder
func GetFramerBuilder(name string) codec.FramerBuilder {
	return framerBuilders[name]
}

// ListenAndServe 包函数 启动默认server transport
func ListenAndServe(opts ...ListenServeOption) error {
	return DefaultServerTransport.ListenAndServe(context.Background(), opts...)
}

// RoundTrip 包函数 启动默认client transport
func RoundTrip(ctx context.Context, req []byte, opts ...RoundTripOption) ([]byte, error) {
	return DefaultClientTransport.RoundTrip(ctx, req, opts...)
}
