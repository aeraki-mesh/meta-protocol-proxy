package transport

// ClientTransportOptions client transport可选参数
type ClientTransportOptions struct {
	UDPRecvSize int
}

// ClientTransportOption client transport option function helper
type ClientTransportOption func(*ClientTransportOptions)

// WithClientUDPRecvSize 设置客户端UDP收包大小
func WithClientUDPRecvSize(size int) ClientTransportOption {
	return func(opts *ClientTransportOptions) {
		opts.UDPRecvSize = size
	}
}

func defaultClientTransportOptions() *ClientTransportOptions {
	return &ClientTransportOptions{UDPRecvSize: 65535}
}
