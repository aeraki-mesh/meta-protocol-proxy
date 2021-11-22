package main

import (
	"git.code.oa.com/trpc-go/trpc-go/log"

	proto "git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command/command_proto"
	trpc "git.code.oa.com/trpc-go/trpc-go"

	_ "git.code.oa.com/trpc-go/trpc-codec/videopacket"
)

func main() {

	log.Debug("server start")
	s := trpc.NewServer()

	imp := new(proto.DemoServer)
	if err := proto.RegisterGreetingService(s, imp); err != nil {
		panic(err)

	}

	err := s.Serve()
	if err != nil {
		panic(err)
	}

}
