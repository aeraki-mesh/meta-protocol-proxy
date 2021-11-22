package transport

import (
	"context"
	"crypto/tls"
	"crypto/x509"
	"errors"
	"fmt"
	"io/ioutil"
	"net"
	"runtime"
	"time"

	reuseport "git.code.oa.com/trpc-go/go_reuseport"
)

// DefaultServerTransport ServerTransport默认实现
var DefaultServerTransport = NewServerTransport(WithReusePort(true))

// NewServerTransport new出来server transport实现
func NewServerTransport(opt ...ServerTransportOption) ServerTransport {

	// option 默认值
	opts := &ServerTransportOptions{
		RecvUDPPacketBufferSize: 65536,
		SendMsgChannelSize:      100,
		RecvMsgChannelSize:      100,
		IdleTimeout:             time.Minute,
	}

	for _, o := range opt {
		o(opts)
	}

	return &serverTransport{opts: opts}
}

// serverTransport server transport具体实现 包括tcp udp serving
type serverTransport struct {
	opts *ServerTransportOptions
}

// ListenAndServe 启动监听，如果监听失败则返回错误
func (s *serverTransport) ListenAndServe(ctx context.Context, opts ...ListenServeOption) error {

	lsopts := &ListenServeOptions{}
	for _, opt := range opts {
		opt(lsopts)
	}

	if lsopts.Listener != nil {
		return s.listenAndServeStream(ctx, lsopts)
	}

	switch lsopts.Network {
	case "tcp", "tcp4", "tcp6":
		return s.listenAndServeStream(ctx, lsopts)
	case "udp", "udp4", "udp6":
		return s.listenAndServePacket(ctx, lsopts)
	default:
		return fmt.Errorf("server transport: not support network type %s", lsopts.Network)
	}
}

// ---------------------------------stream server-----------------------------------------//

func (s *serverTransport) getStreamTLSConfig(opts *ListenServeOptions) (*tls.Config, error) {
	var err error
	tlsConf := &tls.Config{}

	if len(opts.CACertFile) != 0 { // 验证客户端证书
		tlsConf.ClientAuth = tls.RequireAndVerifyClientCert
		if opts.CACertFile != "root" {
			ca, err := ioutil.ReadFile(opts.CACertFile)
			if err != nil {
				return nil, fmt.Errorf("Read ca cert file error:%v", err)
			}
			pool := x509.NewCertPool()
			if !pool.AppendCertsFromPEM(ca) {
				return nil, fmt.Errorf("AppendCertsFromPEM fail")
			}
			tlsConf.ClientCAs = pool
		}
	}
	cert, err := tls.LoadX509KeyPair(opts.TLSCertFile, opts.TLSKeyFile)
	if err != nil {
		return nil, err
	}
	tlsConf.Certificates = []tls.Certificate{cert}
	return tlsConf, nil
}

// getListener 获取 listener
func (s *serverTransport) getListener(opts *ListenServeOptions) (net.Listener, error) {
	var listener = opts.Listener
	var err error
	var tlsConf *tls.Config

	if listener != nil {
		return listener, nil
	}

	// 端口重用，内核分发IO ReadReady事件到多核多线程，加速IO效率
	if s.opts.ReusePort {
		listener, err = reuseport.Listen(opts.Network, opts.Address)
		if err != nil {
			return nil, fmt.Errorf("tcp reuseport error:%v", err)
		}
	} else {
		listener, err = net.Listen(opts.Network, opts.Address)
		if err != nil {
			return nil, err
		}
	}

	// 启用TLS
	if len(opts.TLSCertFile) > 0 && len(opts.TLSKeyFile) > 0 {
		tlsConf, err = s.getStreamTLSConfig(opts)
		if err != nil {
			return nil, err
		}
	}

	if tlsConf != nil {
		listener = tls.NewListener(listener, tlsConf)
	}

	return listener, nil
}

// listenAndServeStream 启动监听，如果监听失败则返回错误
func (s *serverTransport) listenAndServeStream(ctx context.Context,
	opts *ListenServeOptions) error {

	if opts.FramerBuilder == nil {
		return errors.New("tcp transport FramerBuilder empty")
	}

	listener, err := s.getListener(opts)
	if err != nil {
		return err
	}

	go s.serveStream(ctx, listener, opts)

	return nil
}

func (s *serverTransport) serveStream(ctx context.Context, ln net.Listener,
	opts *ListenServeOptions) error {
	return s.serveTCP(ctx, ln, opts)
}

// ---------------------------------packet server-----------------------------------------//

// listenAndServePacket 启动监听，如果监听失败则返回错误
func (s *serverTransport) listenAndServePacket(ctx context.Context,
	opts *ListenServeOptions) error {

	// 端口重用，内核分发IO ReadReady事件到多核多线程，加速IO效率
	if s.opts.ReusePort {
		reuseport.ListenerBacklogMaxSize = 4096
		cores := runtime.NumCPU()
		for i := 0; i < cores; i++ {
			udpconn, err := reuseport.ListenPacket(opts.Network, opts.Address)
			if err != nil {
				return fmt.Errorf("udp reuseport error:%v", err)
			}
			go s.servePacket(ctx, udpconn, opts)
		}
	} else {
		udpconn, err := net.ListenPacket(opts.Network, opts.Address)
		if err != nil {
			return err
		}
		go s.servePacket(ctx, udpconn, opts)
	}

	return nil
}

func (s *serverTransport) servePacket(ctx context.Context, rwc net.PacketConn,
	opts *ListenServeOptions) error {

	switch rwc := rwc.(type) {
	case *net.UDPConn:
		return s.serveUDP(ctx, rwc, opts)
	default:
		return errors.New("transport not support PacketConn impl")
	}
}

// ------------------------tcp/udp connection通用结构 统一处理----------------------------//

func (s *serverTransport) newConn(ctx context.Context, opts *ListenServeOptions) *conn {

	return &conn{
		ctx:         ctx,
		handler:     opts.Handler,
		idleTimeout: s.opts.IdleTimeout,
	}
}

type conn struct {
	ctx         context.Context
	cancelCtx   context.CancelFunc
	idleTimeout time.Duration
	lastVisited time.Time

	handler Handler
}

func (c *conn) handle(ctx context.Context, req []byte) ([]byte, error) {
	return c.handler.Handle(ctx, req)
}
