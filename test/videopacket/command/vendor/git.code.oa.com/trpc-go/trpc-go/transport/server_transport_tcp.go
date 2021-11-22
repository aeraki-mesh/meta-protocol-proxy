package transport

import (
	"context"
	"errors"
	"io"
	"net"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/log"
	"git.code.oa.com/trpc-go/trpc-go/metrics"
	"git.code.oa.com/trpc-go/trpc-go/pool/objectpool"
)

var defaultRecvBufSize = 4096
var bytesPool = objectpool.NewBytesPool(defaultRecvBufSize)

func (s *serverTransport) serveTCP(ctx context.Context, ln net.Listener,
	opts *ListenServeOptions) error {

	defer ln.Close()

	var tempDelay time.Duration
	for {

		select {
		case <-ctx.Done():
			return errors.New("recv server close event")
		default:
		}

		rwc, err := ln.Accept()
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

		if tcpConn, ok := rwc.(*net.TCPConn); ok {
			err = tcpConn.SetKeepAlive(true)
			if err != nil {
				log.Tracef("tcp conn set keepalive error:%v", err)
			}

			if s.opts.KeepAlivePeriod > 0 {
				err = tcpConn.SetKeepAlivePeriod(s.opts.KeepAlivePeriod)
				if err != nil {
					log.Tracef("tcp conn set keepalive period error:%v", err)
				}
			}
		}

		tc := &tcpconn{
			conn:        s.newConn(ctx, opts),
			rwc:         rwc,
			fr:          opts.FramerBuilder.New(rwc),
			remoteAddr:  rwc.RemoteAddr(),
			localAddr:   rwc.LocalAddr(),
			serverAsync: opts.ServerAsync,
			isClosed:    false,
		}

		go tc.serve()
	}
}

type tcpconn struct {
	*conn
	rwc         net.Conn
	fr          codec.Framer
	localAddr   net.Addr
	remoteAddr  net.Addr
	serverAsync bool
	isClosed    bool
}

func (c *tcpconn) genRequestHandler(req []byte) func() {
	//因为req是framebuilder产生的，会被下次读包覆盖
	//如果是同步请求就直接使用req，如果采用协程异步，就需要拷贝内存，防止被覆盖
	var reqCopy []byte
	if c.serverAsync {
		reqCopy = make([]byte, len(req), len(req))
		copy(reqCopy, req)
	} else {
		reqCopy = req
	}
	return func() {
		ctx, msg := codec.WithNewMessage(context.Background())
		defer codec.PutBackMessage(msg)

		// 记录LocalAddr和RemoteAddr到Context
		msg.WithLocalAddr(c.localAddr)
		msg.WithRemoteAddr(c.remoteAddr)
		rsp, err := c.handle(ctx, reqCopy)

		if err != nil {
			if err != errs.ErrServerNoResponse {
				metrics.Counter("trpc.TcpServerTransportHandleFail").Incr()
				log.Trace("transport: tcpconn serve handle fail ", err)
			}
			c.isClosed = true
			return
		}
		if _, err = c.rwc.Write(rsp); err != nil {
			metrics.Counter("trpc.TcpServerTransportWriteFail").Incr()
			log.Trace("transport: tcpconn write fail ", err)
			c.isClosed = true
			return
		}
	}
}

func (c *tcpconn) serve() {
	defer c.rwc.Close()
	for !c.isClosed {

		// 检查上游是否关闭
		select {
		case <-c.ctx.Done():
			return
		default:
		}

		if c.idleTimeout > 0 {
			now := time.Now()
			if now.Sub(c.lastVisited) > 5*time.Second { // SetReadDeadline性能损耗较严重，每5s才更新一次timeout
				c.lastVisited = now
				err := c.rwc.SetReadDeadline(now.Add(c.idleTimeout))
				if err != nil {
					log.Trace("transport: tcpconn SetReadDeadline fail ", err)
					return
				}
			}
		}

		req, err := c.fr.ReadFrame()
		if err != nil {
			if err == io.EOF {
				metrics.Counter("trpc.TcpServerTransportReadEOF").Incr() // 客户端主动断开连接
				return
			}
			if e, ok := err.(net.Error); ok && e.Timeout() { // 客户端超过空闲时间没有发包，服务端主动超时关闭
				metrics.Counter("trpc.TcpServerTransportIdleTimeout").Incr()
				return
			}
			metrics.Counter("trpc.TcpServerTransportReadFail").Incr()
			log.Trace("transport: tcpconn serve ReadFrame fail ", err)
			return
		}
		handler := c.genRequestHandler(req)
		// 生成新的空的通用消息结构数据，并保存到ctx里面
		if c.serverAsync {
			go handler()
		} else {
			handler()
		}

	}

}
