package main

import (
	"context"
	"flag"
	"testing"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/client"
	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/filter"
	"git.code.oa.com/trpc-go/trpc-go/log"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"

	proto "git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command/command_proto"
	p "git.code.oa.com/videocommlib/videopacket-go"

	_ "git.code.oa.com/trpc-go/trpc-codec/videopacket"
	_ "git.code.oa.com/trpc-go/trpc-go"
)

// common options
var addr = flag.String("addr", "ip://127.0.0.1:8000",
	"addr, supporting ip://<ip>:<port>, l5://mid:cid, cmlb://appid[:sysid]")
var cmd = flag.String("cmd", "Get", ", Add/Get")

func TestMain(m *testing.M) {
	flag.Parse()

	ctx, cancel := context.WithTimeout(context.TODO(), time.Millisecond*2000)
	defer cancel()

	node := &registry.Node{}
	command := uint16(0x1)
	proxy := proto.NewGreetingProxy("TestDemo")
	packet := p.NewVideoPacket()
	packet2 := p.NewVideoPacket()
	packet.CommHeader.BasicInfo.Command = int16(command)
	req := &proto.GetReq{
		A: 100,
		B: 200,
	}

	rsp, err := proxy.Get(ctx, req, client.WithReqHead(packet),
		client.WithRspHead(packet2),
		client.WithProtocol("videopacket"),
		client.WithNetwork("tcp4"),
		client.WithTarget(*addr),
		//		client.WithServiceName("TestDemo"),
		client.WithSelectorNode(node),
		client.WithSerializationType(codec.SerializationTypeJCE),
		client.WithFilter(func(ctx context.Context,
			req interface{}, rsp interface{}, f filter.HandleFunc) (err error) {
			log.Debugf("filter req: %v", req)

			err1 := f(ctx, req, rsp)
			if err1 != nil {
				log.Error("filter process with err: %s", err1.Error())
			}
			log.Debugf("filter resp: %v", rsp)
			log.Infof("filter rsp: %p", rsp)
			return err1
		}),
	)
	log.Debugf("rsp: %#v", rsp)
	log.Debugf("rsp: %p", rsp)
	log.Debugf("err:%+v, req:%+v, rsp:%#v, node:%+v, rsphead:%+v",
		err, req, rsp, node, packet2)
	if err != nil {
		panic(err)
	}

	//req.B = req.A
	rsp, err = proxy.Get(ctx, req, client.WithReqHead(packet),
		client.WithRspHead(packet2),
		client.WithProtocol("videopacket"),
		client.WithNetwork("tcp4"),
		client.WithTarget(*addr),
		//		client.WithServiceName("TestDemo"),
		client.WithSelectorNode(node),
		client.WithSerializationType(codec.SerializationTypeJCE),
		client.WithFilter(func(ctx context.Context,
			req interface{}, rsp interface{}, f filter.HandleFunc) (err error) {
			log.Debugf("filter req: %v", req)
			log.Debugf("filter resp: %v", rsp)

			err1 := f(ctx, req, rsp)
			if err1 != nil {
				log.Error("filter process with err: %s", err1.Error())
			}
			return err1
		}),
	)

	log.Debugf("err:%+v, req:%+v, rsp:%+v, node:%+v, rsphead:%+v",
		err, req, rsp, node, packet2)

}
