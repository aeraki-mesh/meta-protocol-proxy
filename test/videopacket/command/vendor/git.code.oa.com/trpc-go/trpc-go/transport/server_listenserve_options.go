package transport

import (
	"net"

	"git.code.oa.com/trpc-go/trpc-go/codec"
)

// ListenServeOptions server每次启动参数
type ListenServeOptions struct {
	Address       string
	Network       string
	Handler       Handler
	FramerBuilder codec.FramerBuilder
	Listener      net.Listener

	CACertFile  string // ca证书
	TLSCertFile string // server证书
	TLSKeyFile  string // server秘钥
	ServerAsync bool //服务端启用异步处理
}

// ListenServeOption function type for config listenServeOptions
type ListenServeOption func(*ListenServeOptions)

// WithServerFramerBuilder 设置FramerBuilder
func WithServerFramerBuilder(fb codec.FramerBuilder) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.FramerBuilder = fb
	}
}

// WithListenAddress 设置ListenAddress
func WithListenAddress(address string) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.Address = address
	}
}

// WithListenNetwork 设置ListenNetwork
func WithListenNetwork(network string) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.Network = network
	}
}

// WithListener 允许用户自己设置listener，用于自己操作accept read/write等特殊逻辑
func WithListener(lis net.Listener) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.Listener = lis
	}
}

// WithHandler 设置业务处理抽象接口Handler
func WithHandler(handler Handler) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.Handler = handler
	}
}

// WithServeTLS 设置服务支持TLS
func WithServeTLS(certFile, keyFile, caFile string) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.TLSCertFile = certFile
		opts.TLSKeyFile = keyFile
		opts.CACertFile = caFile
	}
}

// WithServerAsync 设置服务端异步处理
// 其他框架调用trpc，比如TAF调用的时候会使用长连接，这个时候TRPC服务端不能并发处理，导致超时
// 该从监听选项一直传递到每个TCP连接
func WithServerAsync(serverAsync bool) ListenServeOption {
	return func(opts *ListenServeOptions) {
		opts.ServerAsync = serverAsync
	}
		
} 