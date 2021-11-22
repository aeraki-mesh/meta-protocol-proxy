# tRPC-Go 业务协议打解包实现
- tRPC-Go 可以支持任意的第三方业务通信协议，只需要实现codec相关接口即可。
- 每个业务协议单独一个go module，互不影响，go get只会拉取需要的codec模块。
- 业务协议一般有两种典型样式：IDL协议 如tars，非IDL协议 如oidb，具体情况可以分别参考tars和oidb的实现。

## 具体业务协议实现仓库：https://git.code.oa.com/trpc-go/trpc-codec


# 相关概念解析
- Message：每个请求的通用消息体，为了支持任意的第三方协议，trpc抽象出了message这个通用数据结构来携带框架需要的基本信息。
- Codec：业务协议打解包接口，业务协议分为 包头 和 包体，这里只需要解析出二进制包体即可，一般包头放在msg里面，业务不用关心。
```golang
type Codec interface {
    //server解包 从完整的二进制网络数据包解析出二进制请求包体
    Decode(message Msg, request-buffer []byte) (reqbody []byte, err error)
    //server回包 把二进制响应包体打包成一个完整的二进制网络数据
    Encode(message Msg, rspbody []byte) (response-buffer []byte, err error)
}
```
- Serializer：body序列化接口，目前支持 protobuf json jce。可插拔，用户可自己定义并注册进来。
```golang
type Serializer interface {
    //server解包出二进制包体后，调用该函数解析到具体的reqbody结构体
    Unmarshal(req-body-bytes []byte, reqbody interface{}) error
    //server回包rspbody结构体，调用该函数转成二进制包体
    Marshal(rspbody interface{}) (rsp-body-bytes []byte, err error)
}
```
- Compressor：body解压缩方式，目前支持 gzip snappy。可插拔，用户可自己定义并注册进来。
```golang
type Compressor interface {
    //server解出二进制包体，调用该函数，解压出原始二进制数据
	Decompress(in []byte) (out []byte, err error)
	//server回包二进制包体，调用该函数，压缩成小的二进制数据
	Compress(in []byte) (out []byte, err error)
}
```

# 具体实现步骤（可参考[trpc-codec](https://git.code.oa.com/trpc-go/trpc-go/blob/master/codec.go)）
- 1. 实现tRPC-Go [FrameBuilder拆包接口](https://git.code.oa.com/trpc-go/trpc-go/blob/master/transport/transport.go), 拆出一个完整的消息包。
- 2. 实现tRPC-Go [Codec打解包接口](https://git.code.oa.com/trpc-go/trpc-go/blob/master/codec/codec.go)，需要注意以下几点：
 - server codec decode收请求包后，需要通过 msg.WithServerRPCName msg.WithRequestTimeout 告诉trpc如何分发路由以及指定上游服务的剩余超时时间。
 - server codec encode回响应包前，需要通过 msg.ServerRspErr 将handler处理函数错误返回error转成具体的业务协议包头错误码。
 - client codec encode发请求包前，需要通过 msg.ClientRPCName msg.RequestTimeout 指定请求路由及告诉下游服务剩余超时时间。
 - client codec decode收响应包后，需要通过 errs.New 将具体业务协议错误码转换成err返回给用户调用函数。
- 3. init函数将具体实现注册到trpc框架中。