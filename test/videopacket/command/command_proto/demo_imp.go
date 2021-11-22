package hello

import (
	"context"
	"errors"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/log"

	p "git.code.oa.com/videocommlib/videopacket-go"
)

type DemoServer struct{}

// Get 为 demo 服务 Get 实际逻辑
func (d *DemoServer) Get(ctx context.Context, req *GetReq, rsp *GetRsp) (err error) {
	log.Infof("request get: %v", req)
	msg := codec.Message(ctx)
	head := msg.ServerReqHead()
	videopacketHead, ok := head.(*p.VideoPacket)
	if !ok {
		return errors.New("invalid head type")
	}
	log.Infof("videopacket head.Command: %#v", videopacketHead.CommHeader.BasicInfo.Command)
	rsp.C = req.A + req.B
	return nil
}
