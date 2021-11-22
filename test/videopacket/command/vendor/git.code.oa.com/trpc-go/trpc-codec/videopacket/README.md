# tRPC-Go  第三方协议实现之 videopacket
[![BK Pipelines Status](https://api.bkdevops.qq.com/process/api/external/pipelines/projects/pcgtrpcproject/p-dd5b6c91f1c34b0ca55ea7dfe78dd8d7/badge?X-DEVOPS-PROJECT-ID=pcgtrpcproject)](http://api.devops.oa.com/process/api-html/user/builds/projects/pcgtrpcproject/pipelines/p-dd5b6c91f1c34b0ca55ea7dfe78dd8d7/latestFinished?X-DEVOPS-PROJECT-ID=pcgtrpcproject) [![Coverage](https://tcoverage.woa.com/api/getCoverage/getTotalImg/?pipeline_id=p-dd5b6c91f1c34b0ca55ea7dfe78dd8d7)](http://macaron.oa.com/api/coverage/getTotalLink/?pipeline_id=p-dd5b6c91f1c34b0ca55ea7dfe78dd8d7) [![GoDoc](https://img.shields.io/badge/API%20Docs-GoDoc-green)](http://godoc.oa.com/git.code.oa.com/trpc-go/trpc-codec/videopacket)

## videopacket 协议是腾讯视频产品线使用的一套服务之间的通信协议，基于 jce 进行编码
协议的组成格式和分析可以参考 [腾讯视频videopacket协议理解及业务抓包分析过程](http://km.oa.com/articles/show/311763?kmref=search&from_page=1&no=1)

# 介绍
>本package 提供了 trpc-go 实现一个 videopacket 协议的服务端 and/or 客户端的方法

1. trpc-go 服务调用 videopacket 协议的服务使用此 package 提供的方法
2. videopacket 服务调用 trpc 的服务则按照 trpc 各语言的客户端来调用

# 使用
1. jce 文件定义
```
module Hello
{
    struct GetReq {
        0 optional int a;
        1 optional int b;
    };

    struct GetRsp {
        0 optional int c;
    };

    interface Greeting
    {
        int Get(GetReq req, out GetRsp rsp);
    };
};
```
`!!!`由于 videopacket 协议的包体(CommHead的Body部分与业务相关，用作请求和返回业务数据。所以生成代码只支持上述例子中 Get的方式定义。方法的 int 返回类型将被忽略，与 void 等价)

2. 生成相关代码
* 安装 trpc4videopacket 工具
* 按照工具执行命令生成相关代码
参考 [https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/tools/trpc4videopacket](https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/tools/trpc4videopacket)


# 示例
1. greeting
    [https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/examples/greeting](https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/examples/greeting)
2. 需要按照命令字进行 rpc 路由的请求，需要在函数定义中加命令字的相关注释，格式如下:
```jce
module Hello
{
    struct GetReq {
        0 optional int a;
        1 optional int b;
    };

    struct GetRsp {
        0 optional int c;
    };

    interface Greeting
    {
        int Get(GetReq req, out GetRsp rsp); //command=0xfc91
    };
};
```

示例可以参考: [https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/examples/command](https://git.code.oa.com/trpc-go/trpc-codec/tree/master/videopacket/examples/command)

3. 默认不支持命令字为 0 的命令字函数路由，如果已有业务使用命令字为 0 的函数路由，需要显示调用方法 ForceEnableCommand 强制使用命令字模式
# 常见FAQ
1. client 调用
    此仓库只实现了按照 videopacket 协议的 codec 部分，具体请求的client参数可以参考 trpc-go 的 client 部分[https://git.code.oa.com/trpc-go/trpc-go/tree/master/client](https://git.code.oa.com/trpc-go/trpc-go/tree/master/client) 
2. 服务端获取及设置包头
    ```golang
    package main

import (
	"context"
	"errors"

	proto "git.code.oa.com/trpc-go/trpc-codec/videopacket/examples/command/command_proto"
	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/log"
	p "git.code.oa.com/videocommlib/videopacket-go"
)

type demoServer struct{}

func (d *demoServer) Get(ctx context.Context, Req *proto.GetReq, Rsp *proto.GetRsp) (err error) {
	log.Infof("ctx: %v", ctx)
	log.Infof("request get: %v", Req)
	msg := codec.Message(ctx)
	log.Infof("message: %#v", msg)
	head := msg.ServerReqHead()
	videopacketHead, ok := head.(*p.VideoPacket)
	if !ok {
		return errors.New("invalid head type")
	}
	log.Infof("videopacket head: %#v", videopacketHead)
    rspHead1 := p.NewVideoPacket()
	rspHead1.CommHeader.BasicInfo.Command = 123
	msg.WithServerRspHead(rspHead1)
	Rsp.C = Req.A + Req.B
	return nil
}

    ```


3. ListenAndServe fail:tcp transport FramerBuilder empty
代码中需要加入 import 
```
    _ "git.code.oa.com/trpc-go/trpc-codec/videopacket"
```

4. 服务端无法收到请求包
    检查 trpc_go.yaml 里 service 对应的 protocol 为 videopacket

5. 框架错误与业务错误的区分
   - 服务端设置业务错误码通过设置 videopacket 包头 CommHeader.BasicInfo.Result 字段，并大于1000，因为1000以内是 videopacket 预留
   - 客户端对于一次 RPC 的调用，返回的 err 为 trpc-go/errs [Error](http://pkg.woa.com/git.code.oa.com/trpc-go/trpc-go/errs#Error) 类型根据 Type 来判断
     - 如果是 ErrorTypeBusiness 则为业务错误码
     - 如果是 ErrorTypeCalleeFramework 则为框架错误码
       
