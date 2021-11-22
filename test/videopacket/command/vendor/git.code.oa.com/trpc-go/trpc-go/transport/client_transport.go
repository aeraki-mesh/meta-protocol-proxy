package transport

import (
	"context"
	"fmt"

	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/pool/connpool"
)

// DefaultClientTransport 默认 client transport实现
var DefaultClientTransport = NewClientTransport()

// NewClientTransport new出来的client transport实现
func NewClientTransport(opt ...ClientTransportOption) ClientTransport {

	// option 默认值
	opts := defaultClientTransportOptions()

	// 将传入的func option写到opts字段中
	for _, o := range opt {
		o(opts)
	}

	return &clientTransport{opts: opts}
}

// clientTransport client transport具体实现 包括tcp roundtrip udp roundtrip
type clientTransport struct {
	/*每一个transport的options分成两种。
	一种是ClientTransportOptions 这个是针对transport的option，对所有的RoundTrip请求都生效的，具体实现需要的参数，框架不关心。
	另一种是RoundTripOptions 这个是当次请求的option，如address之类的 不同请求不同值，可配置的，由框架上层传进来。
	*/
	opts *ClientTransportOptions
}

// RoundTrip 发出client请求
func (c *clientTransport) RoundTrip(ctx context.Context, req []byte,
	roundTripOpts ...RoundTripOption) (rsp []byte, err error) {

	// 默认值
	opts := &RoundTripOptions{
		Pool: connpool.DefaultConnectionPool,
	}

	// 将传入的func option写到opts字段中
	for _, o := range roundTripOpts {
		o(opts)
	}

	switch opts.Network {
	case "tcp", "tcp4", "tcp6":
		return c.tcpRoundTrip(ctx, req, opts)
	case "udp", "udp4", "udp6":
		return c.udpRoundTrip(ctx, req, opts)
	default:
		return nil, errs.NewFrameError(errs.RetClientConnectFail,
			fmt.Sprintf("client transport: network %s not support", opts.Network))
	}
}
