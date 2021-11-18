package main

import (
	"context"

	pb "git.code.oa.com/trpcprotocol/test/helloworld"
)

// SayHello ...
func (s *greeterServiceImpl) SayHello(ctx context.Context, req *pb.HelloRequest, rsp *pb.HelloReply) error {
	// implement business logic here ...
	// ...
	rsp.Msg = req.Msg
	return nil
}
