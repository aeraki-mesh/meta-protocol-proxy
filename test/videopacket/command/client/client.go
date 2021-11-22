package main

import (
	"context"
	"flag"
	"fmt"
	p "git.code.oa.com/videocommlib/videopacket-go"

	proto "git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command/command_proto"
	"git.code.oa.com/trpc-go/trpc-go/client"

	_ "git.code.oa.com/trpc-go/trpc-codec/videopacket"
)

var serverAddr string

func init() {
	flag.StringVar(&serverAddr, "server-addr", "127.0.0.1:8000", "videopacket服务端地址，默认：127.0.0.1:8000")
	flag.Parse()
}

func main() {
	proxy := proto.NewGreetingProxy("wonder")

	packet := p.NewVideoPacket()
	packet.CommHeader.BasicInfo.Command = 1
	packet.CommHeader.ServerRoute.ToServerName = "TestDemo"
	packet.CommHeader.ServerRoute.FuncName = "Get"
	rsp, err := proxy.Get(context.Background(), &proto.GetReq{
		A: 1,
		B: 2,
	},
		client.WithReqHead(packet),
		client.WithProtocol("videopacket"),
		//client.WithNetwork("tcp4"),
		client.WithTarget(fmt.Sprintf("ip://%s", serverAddr)),
	)
	if err != nil {
		fmt.Println("err: ", err.Error())
		return
	}
	fmt.Println("rsp: ", rsp.C)
}
