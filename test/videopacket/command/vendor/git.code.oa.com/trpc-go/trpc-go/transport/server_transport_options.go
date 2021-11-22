package transport

import (
	"runtime"
	"time"
)

// ServerTransportOptions server transport参数
type ServerTransportOptions struct {
	RecvMsgChannelSize      int
	SendMsgChannelSize      int
	RecvUDPPacketBufferSize int
	IdleTimeout             time.Duration
	KeepAlivePeriod         time.Duration
	ReusePort               bool
}

// ServerTransportOption server transport option function helper
type ServerTransportOption func(*ServerTransportOptions)

// WithRecvMsgChannelSize 设置ServerTransport tcp请求中recvCh的收包缓冲长度
func WithRecvMsgChannelSize(size int) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.RecvMsgChannelSize = size
	}
}

// WithReusePort 设置ServerTransport 是否开启重用端口
func WithReusePort(reuse bool) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.ReusePort = reuse
		if runtime.GOOS == "windows" {
			options.ReusePort = false
		}
	}
}

// WithSendMsgChannelSize 设置ServerTransport tcp请求中sendCh的发包缓冲长度
func WithSendMsgChannelSize(size int) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.SendMsgChannelSize = size
	}
}

// WithRecvUDPPacketBufferSize 设置ServerTransport udp请求中接收请求数据预分配buffer大小
func WithRecvUDPPacketBufferSize(size int) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.RecvUDPPacketBufferSize = size
	}
}

// WithIdleTimeout 设置Server端连接空闲存在时间
func WithIdleTimeout(timeout time.Duration) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.IdleTimeout = timeout
	}
}

// WithKeepAlivePeriod 设置tcp长连接保活时间间隔
// TCP TLS 设置无效,因为tls库底层不是net.TCPConn，也没有暴露net.Conn
func WithKeepAlivePeriod(d time.Duration) ServerTransportOption {
	return func(options *ServerTransportOptions) {
		options.KeepAlivePeriod = d
	}
}
