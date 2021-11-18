package main

import (
	"context"
	"testing"
	"time"

	_ "git.code.oa.com/trpc-go/trpc-filter/debuglog"
	_ "git.code.oa.com/trpc-go/trpc-filter/recovery"
	trpc "git.code.oa.com/trpc-go/trpc-go"
	"git.code.oa.com/trpc-go/trpc-go/client"
	_ "git.code.oa.com/trpc-go/trpc-metrics-runtime"
	pb "git.code.oa.com/trpcprotocol/test/helloworld"
	_ "go.uber.org/automaxprocs"
)

var (
	defaultAddr = "ip://127.0.0.1:8000"
	proxyAddr   = "ip://127.0.0.1:28000"
)

// init
func init() {
	trpc.ServerConfigPath = "trpc_go.yaml"
	s := trpc.NewServer()
	pb.RegisterGreeterService(s, &greeterServiceImpl{})
	go s.Serve()
}

// BenchmarkTrpcProxy
func BenchmarkTrpcProxy(b *testing.B) {
	for i := 0; i < b.N; i++ {
		sayhello(proxyAddr)
	}
}

// BenchmarkTrpcNoProxy
func BenchmarkTrpcNoProxy(b *testing.B) {
	for i := 0; i < b.N; i++ {
		sayhello(defaultAddr)
	}
}

// BenchmarkTrpcNoProxyGoroutines
func BenchmarkTrpcNoProxyGoroutines(b *testing.B) {
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			sayhello(defaultAddr)
		}
	})
}

// BenchmarkTrpcProxyGoroutines
func BenchmarkTrpcProxyGoroutines(b *testing.B) {
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			sayhello(proxyAddr)
		}
	})
}

// sayhello
func sayhello(addr string) {
	rsphead := &trpc.ResponseProtocol{}
	opts := []client.Option{
		client.WithServiceName("trpc.test.helloworld.Greeter"),
		client.WithProtocol("trpc"),
		client.WithNetwork("tcp4"),
		client.WithTarget(addr),
		client.WithRspHead(rsphead),
	}
	proxy := pb.NewGreeterClientProxy()
	msg := "hello"
	ctx, cancel := context.WithTimeout(context.Background(), time.Millisecond*2000)
	defer cancel()
	req := &pb.HelloRequest{
		Msg: msg,
	}
	rsp, err := proxy.SayHello(ctx, req, opts...)
	if err != nil {
		panic(err)
	}
	_ = rsp
	// b.Logf("len_req:%d, req:%s, len_rsp:%d, rsp:%s, err:%v, rsphead:%s", len(req.Msg), req.Msg, len(rsp.Msg),
	// rsp.Msg, err, rsphead)

}
