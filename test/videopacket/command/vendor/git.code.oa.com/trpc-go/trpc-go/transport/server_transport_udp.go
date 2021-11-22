package transport

import (
	"context"
	"errors"
	"net"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/log"
	"git.code.oa.com/trpc-go/trpc-go/metrics"
)

func (s *serverTransport) serveUDP(ctx context.Context, rwc *net.UDPConn, opts *ListenServeOptions) error {

	var tempDelay time.Duration
	recvBuf := make([]byte, s.opts.RecvUDPPacketBufferSize)
	for {

		select {
		case <-ctx.Done():
			return errors.New("recv server close event")
		default:
		}

		num, raddr, err := rwc.ReadFromUDP(recvBuf)
		if err != nil {
			if ne, ok := err.(net.Error); ok && ne.Temporary() {
				if tempDelay == 0 {
					tempDelay = 5 * time.Millisecond
				} else {
					tempDelay *= 2
				}
				if max := 1 * time.Second; tempDelay > max {
					tempDelay = max
				}
				time.Sleep(tempDelay)
				continue
			}
			return err
		}
		tempDelay = 0

		c := &udpconn{
			conn:       s.newConn(ctx, opts),
			req:        make([]byte, num),
			rwc:        rwc,
			remoteAddr: raddr,
		}
		copy(c.req, recvBuf[:num])
		go c.serve()
	}
}

type udpconn struct {
	*conn
	req        []byte
	rwc        *net.UDPConn
	remoteAddr *net.UDPAddr
}

func (c *udpconn) serve() {

	// 生成新的空的通用消息结构数据，并保存到ctx里面
	ctx, msg := codec.WithNewMessage(context.Background())
	defer codec.PutBackMessage(msg)

	// 记录LocalAddr和RemoteAddr到Context
	msg.WithLocalAddr(c.rwc.LocalAddr())
	msg.WithRemoteAddr(c.remoteAddr)

	rsp, err := c.handle(ctx, c.req)
	if err != nil {
		if err != errs.ErrServerNoResponse {
			metrics.Counter("trpc.UdpServerTransportHandleFail").Incr()
			log.Tracef("udp handle fail:%v", err)
		}
		return
	}

	_, err = c.rwc.WriteToUDP(rsp, c.remoteAddr)
	if err != nil {
		metrics.Counter("trpc.UdpServerTransportWriteFail").Incr()
		log.Tracef("udp write out fail:%v", err)
		return
	}
}
