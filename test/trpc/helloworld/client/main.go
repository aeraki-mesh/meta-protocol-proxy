package main

import (
	"context"
	"flag"
	"fmt"
	"net/http"
	_ "net/http/pprof"
	"sync"
	"sync/atomic"
	"time"

	trpc "git.code.oa.com/trpc-go/trpc-go"
	"git.code.oa.com/trpc-go/trpc-go/client"
	"git.code.oa.com/trpc-go/trpc-go/log"
	pb "git.code.oa.com/trpcprotocol/test/helloworld"
)

var addr = flag.String("addr", "ip://127.0.0.1:8000",
	"addr, supporting ip://<ip>:<port>, l5://mid:cid, cmlb://appid[:sysid]")
var cmd = flag.String("cmd", "SayHello", "cmd SayHello")
var max = flag.Int64("n", 1, "-n 10 number for requests")
var interval = flag.Int("i", 1000, "-i 10, send interval")
var concurrency = flag.Int("c", 1, "-c 10, concurrency number")
var wg sync.WaitGroup

func main() {
	flag.Parse()
	count := int64(0)
	atomicNum := int64(0)
	wg.Add(*concurrency)
	tmp := *concurrency
	begin := time.Now()

	go func() {
		http.ListenAndServe("0.0.0.0:8081", nil)
	}()

	for {
		if tmp <= 0 {
			break
		}
		tmp = tmp - 1
		go func() {
			rsphead := &trpc.ResponseProtocol{}
			opts := []client.Option{
				client.WithServiceName("trpc.test.helloworld"),
				client.WithProtocol("trpc"),
				client.WithNetwork("tcp4"),
				client.WithTarget(*addr),
				client.WithRspHead(rsphead),
			}
			proxy := pb.NewGreeterClientProxy()
			switch *cmd {
			case "SayHello":
				msg := "trpc-go-client"
				for {
					count++
					if count > *max {
						break
					}
					tmsg := msg + fmt.Sprintf("-%d", atomic.AddInt64(&atomicNum, 1))
					ctx, cancel := context.WithTimeout(context.Background(), time.Millisecond*2000)
					defer cancel()
					req := &pb.HelloRequest{
						Msg: tmsg,
					}
					rsp, err := proxy.SayHello(ctx, req, opts...)
					if err != nil {
						log.Error(err)
						continue
					}
					log.Debugf("len_req:%d, req:%s, len_rsp:%d, rsp:%s, err:%v, rsphead:%s", len(req.Msg), req.Msg,
						len(rsp.Msg), rsp.Msg, err, rsphead)

					if tmsg != rsp.Msg {
						log.Fatalf("%s != %s", tmsg, rsp.Msg)
					}
					if *interval > 0 {
						time.Sleep(time.Millisecond * time.Duration(*interval))
					}
				}
			}
			wg.Done()
		}()
	}
	wg.Wait()
	since := time.Since(begin)
	val := since.Milliseconds()
	if val > 0 {
		log.Debugf("use time:%d ms, qps: %.6f/s ", val, float64(*max)/float64(val*1000.0))
	}
}
