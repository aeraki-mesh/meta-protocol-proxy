package transport

import (
	"context"
	"net"

	"git.code.oa.com/trpc-go/trpc-go/errs"
)

// udpRoundTrip 发送udp请求
func (c *clientTransport) udpRoundTrip(ctx context.Context, reqData []byte,
	opts *RoundTripOptions) ([]byte, error) {

	conn, addr, err := c.dialUDP(ctx, opts)
	if err != nil {
		return nil, err
	}
	defer conn.Close()

	if err := c.udpWriteFrame(conn, reqData, addr, opts); err != nil {
		return nil, err
	}

	return c.udpReadFrame(ctx, conn, opts)
}

// udpReadFrame udp 读数据帧
func (c *clientTransport) udpReadFrame(
	ctx context.Context, conn net.PacketConn, opts *RoundTripOptions) ([]byte, error) {
	// 只发不收
	if opts.ReqType == SendOnly {
		return nil, errs.ErrClientNoResponse
	}

	select {
	case <-ctx.Done():
		return nil, errs.NewFrameError(errs.RetClientTimeout, "udp client transport select after Write: "+ctx.Err().Error())
	default:
	}

	// 收包
	recvData := make([]byte, 64*1024)

	num, _, err := conn.ReadFrom(recvData)
	if err != nil {
		if e, ok := err.(net.Error); ok && e.Timeout() {
			return nil, errs.NewFrameError(errs.RetClientTimeout, "udp client transport ReadFrom: "+err.Error())
		}
		return nil, errs.NewFrameError(errs.RetClientNetErr, "udp client transport ReadFrom: "+err.Error())
	}
	if num == 0 {
		return nil, errs.NewFrameError(errs.RetClientNetErr, "udp client transport ReadFrom: num empty")
	}

	return recvData[:num], nil

}

// udpWriteReqData udp 写请求数据
func (c *clientTransport) udpWriteFrame(conn net.PacketConn,
	reqData []byte, addr *net.UDPAddr, opts *RoundTripOptions) error {
	// 发包
	var num int
	var err error
	if opts.ConnectionMode == Connected {
		udpconn := conn.(*net.UDPConn)
		num, err = udpconn.Write(reqData)
	} else {
		num, err = conn.WriteTo(reqData, addr)
	}

	if err != nil {
		if e, ok := err.(net.Error); ok && e.Timeout() {
			return errs.NewFrameError(errs.RetClientTimeout, "udp client transport WriteTo: "+err.Error())
		}
		return errs.NewFrameError(errs.RetClientNetErr, "udp client transport WriteTo: "+err.Error())
	}

	if num != len(reqData) {
		return errs.NewFrameError(errs.RetClientNetErr, "udp client transport WriteTo: num mismatch")
	}

	return nil
}

// dialUDP 建立 udp
func (c *clientTransport) dialUDP(ctx context.Context, opts *RoundTripOptions) (net.PacketConn, *net.UDPAddr, error) {
	addr, err := net.ResolveUDPAddr(opts.Network, opts.Address)
	if err != nil {
		return nil, nil, errs.NewFrameError(errs.RetClientNetErr, "udp client transport ResolveUDPAddr: "+err.Error())
	}

	var conn net.PacketConn

	if opts.ConnectionMode == Connected {
		conn, err = net.DialUDP(opts.Network, nil, addr)
	} else {
		conn, err = net.ListenPacket(opts.Network, ":")
	}

	if err != nil {
		return nil, nil, errs.NewFrameError(errs.RetClientNetErr, "udp client transport Dial: "+err.Error())
	}

	d, ok := ctx.Deadline()
	if ok {
		conn.SetDeadline(d)
	}

	return conn, addr, nil
}
