package main

import (
	_ "git.code.oa.com/trpc-go/trpc-filter/debuglog"
	_ "git.code.oa.com/trpc-go/trpc-filter/recovery"
	trpc "git.code.oa.com/trpc-go/trpc-go"
	admin "git.code.oa.com/trpc-go/trpc-go/admin"
	_ "git.code.oa.com/trpc-go/trpc-metrics-runtime"
	pb "git.code.oa.com/trpcprotocol/test/helloworld"
	_ "go.uber.org/automaxprocs"
)

// greeterServiceImpl struct
type greeterServiceImpl struct{}

func main() {

	s := trpc.NewServer()

	pb.RegisterGreeterService(s, &greeterServiceImpl{})
	go admin.Run()

	s.Serve()
}
