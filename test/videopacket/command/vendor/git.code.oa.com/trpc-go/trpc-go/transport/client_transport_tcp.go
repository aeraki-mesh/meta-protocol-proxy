package transport

import (
	"context"
	"net"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/pool/connpool"
)

// tcpRoundTrip 发送tcp请求 支持 1.send 2. sendAndRcv 3. keepalive 4. multiplex
func (c *clientTransport) tcpRoundTrip(ctx context.Context, reqData []byte,
	opts *RoundTripOptions) ([]byte, error) {

	if opts.Pool == nil {
		return nil, errs.NewFrameError(errs.RetClientConnectFail,
			"tcp client transport: connection pool empty")
	}

	if opts.FramerBuilder == nil {
		return nil, errs.NewFrameError(errs.RetClientConnectFail,
			"tcp client transport: framer builder empty")
	}

	conn, err := c.dialTCP(ctx, opts)
	if err != nil {
		return nil, err
	}
	defer conn.Close() // tcp连接是独占复用的，Close内部会判断是否应该放回连接池继续复用

	if ctx.Err() == context.Canceled {
		return nil, errs.NewFrameError(errs.RetClientCanceled,
			"tcp client transport canceled before Write: "+ctx.Err().Error())
	}
	if ctx.Err() == context.DeadlineExceeded {
		return nil, errs.NewFrameError(errs.RetClientTimeout,
			"tcp client transport timeout before Write: "+ctx.Err().Error())
	}

	if err := c.tcpWriteFrame(ctx, conn, reqData); err != nil {
		return nil, err
	}

	return c.tcpReadFrame(conn, opts)
}

// dialTCP 建立 tcp 连接
func (c *clientTransport) dialTCP(ctx context.Context, opts *RoundTripOptions) (net.Conn, error) {
	var timeout time.Duration
	d, ok := ctx.Deadline()
	if ok {
		timeout = d.Sub(time.Now())
	}
	var conn net.Conn

	if opts.DisableConnectionPool {
		var err error
		conn, err = connpool.Dial(&connpool.DialOptions{
			Network:       opts.Network,
			Address:       opts.Address,
			Timeout:       timeout,
			CACertFile:    opts.CACertFile,
			TLSCertFile:   opts.TLSCertFile,
			TLSKeyFile:    opts.TLSKeyFile,
			TLSServerName: opts.TLSServerName,
		})

		if err != nil {
			return nil, errs.NewFrameError(errs.RetClientConnectFail,
				"tcp client transport dial: "+err.Error())
		}
	} else {
		var err error
		// 从连接池中获取连接
		conn, err = opts.Pool.Get(opts.Network, opts.Address, timeout,
			connpool.WithFramerBuilder(opts.FramerBuilder),
			connpool.WithDialTLS(opts.TLSCertFile, opts.TLSKeyFile, opts.CACertFile, opts.TLSServerName))

		if err != nil {
			return nil, errs.NewFrameError(errs.RetClientConnectFail,
				"tcp client transport connection pool: "+err.Error())
		}
	}

	if ok {
		conn.SetDeadline(d)
	}

	return conn, nil
}

// tcpWriteReqData tcp 写请求数据
func (c *clientTransport) tcpWriteFrame(ctx context.Context, conn net.Conn, reqData []byte) error {
	// 循环发包
	sentNum := 0
	num := 0
	var err error
	for sentNum < len(reqData) {

		num, err = conn.Write(reqData[sentNum:])
		if err != nil {
			if e, ok := err.(net.Error); ok && e.Timeout() {
				return errs.NewFrameError(errs.RetClientTimeout,
					"tcp client transport Write: "+err.Error())
			}
			return errs.NewFrameError(errs.RetClientNetErr,
				"tcp client transport Write: "+err.Error())
		}

		sentNum += num

		if ctx.Err() == context.Canceled {
			return errs.NewFrameError(errs.RetClientCanceled,
				"tcp client transport canceled after Write: "+ctx.Err().Error())
		}
		if ctx.Err() == context.DeadlineExceeded {
			return errs.NewFrameError(errs.RetClientTimeout,
				"tcp client transport timeout after Write: "+ctx.Err().Error())
		}
	}

	return nil
}

// tcpReadFrame tcp 读数据帧
func (c *clientTransport) tcpReadFrame(conn net.Conn, opts *RoundTripOptions) ([]byte, error) {
	// 只发不收
	if opts.ReqType == SendOnly {
		return nil, errs.ErrClientNoResponse
	}

	var fr codec.Framer
	if opts.DisableConnectionPool {
		// 禁用连接池每个链接需要新建 Framer
		fr = opts.FramerBuilder.New(conn)
	} else {
		// 连接池中的连接 Framer 和 conn 绑定的
		var ok bool
		fr, ok = conn.(codec.Framer)
		if !ok {
			return nil, errs.NewFrameError(errs.RetClientConnectFail,
				"tcp client transport: framer not implemented")
		}
	}

	rspData, err := fr.ReadFrame()
	if err != nil {
		if e, ok := err.(net.Error); ok && e.Timeout() {
			return nil, errs.NewFrameError(errs.RetClientTimeout,
				"tcp client transport ReadFrame: "+err.Error())
		}
		return nil, errs.NewFrameError(errs.RetClientNetErr,
			"tcp client transport ReadFrame: "+err.Error())
	}

	return rspData, nil
}
