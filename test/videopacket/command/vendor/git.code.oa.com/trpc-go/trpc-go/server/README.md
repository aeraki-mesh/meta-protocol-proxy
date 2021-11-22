# tRPC-Go 进程服务

## 服务端调用模式
```golang
type greeterServerImpl struct{}

func (s *greeterServerImpl) SayHello(ctx context.Context, req *pb.HelloRequest, rsp *pb.HelloReply) error {
	// implement business logic here ...
	// ...

	return nil
}

func main() {
	
	s := trpc.NewServer()
	
	pb.RegisterGreeterServer(s, &greeterServerImpl{})
	
	s.Serve()
}
```

## 相关概念解析
 - server：代表一个服务实例，即 一个进程（只有一个service的server也可以当作service来处理）
 - service：代表一个逻辑服务，即 一个真正监听端口的对外服务，与配置文件的service一一对应，一个server可能包含多个service，一个端口一个service
 - proto service：代表一个协议描述服务，protobuf协议文件里面定义的service，通常service与proto service是一一对应的，也可由用户自己通过Register任意组合

## service映射关系

 - 假如协议文件写了多个service，如:
   ```pb
    service hello {
        rpc SayHello(Request) returns (Response) {};
    }
    service bye {
        rpc SayBye(Request) returns (Response) {};
    }
   ```
 - 配置文件也写了多个service，如：
   ```yaml
   server:                                             #服务端配置
      app: test                                        #业务的应用名
      server: helloworld                               #进程服务名
      service:                                         #业务服务提供的service，可以有多个
        - name: trpc.test.helloworld.Greeter1          #service的路由名称
          ip: 127.0.0.1                                #服务监听ip地址 可使用占位符 ${ip},ip和nic二选一，优先ip
          port: 8000                                   #服务监听端口 可使用占位符 ${port}
          protocol: trpc                               #应用层协议 trpc http
        - name: trpc.test.helloworld.Greeter2          #service的路由名称
          ip: 127.0.0.1                                #服务监听ip地址 可使用占位符 ${ip},ip和nic二选一，优先ip
          port: 8080                                   #服务监听端口 可使用占位符 ${port}
          protocol: http                               #应用层协议 trpc http
   ```
 - 首先创建一个server，svr := trpc.NewServer()，配置文件定义了多少个service，就会启动多少个service逻辑服务
 - 组合方式：
  - 单个proto service注册到server里面：pb.RegisterHelloServer(svr, helloImpl) 这里会将协议文件内部的hello server desc注册到server内部的所有service里面
  - 单个proto service注册到service里面：pb.RegisterByeServer(svr.Service("trpc.test.helloworld.Greeter1"), byeImpl) 这里只会将协议文件内部的bye server desc注册到指定service name的service里面
  - 多个proto service注册到同一个service里面：pb.RegisterHelloServer(svr.Service("trpc.test.helloworld.Greeter1"), helloImpl) pb.RegisterByeServer(svr.Service("trpc.test.helloworld.Greeter1"), byeImpl)，这个Greeter1逻辑service同时支持处理不同的协议service处理函数

## 服务端执行流程
- 1. accept一个新链接启动一个goroutine接收该链接数据
- 2. 收到一个完整数据包，解包整个请求
- 3. 查询handler map，定位到具体处理函数
- 4. 解压请求body
- 5. 设置消息整体超时
- 6. 反序列化请求body
- 7. 调用前置拦截器
- 8. 调用业务处理函数
- 9. 调用后置拦截器
- 10. 序列化响应body
- 11. 压缩响应body
- 12. 打包整个响应
- 13. 回包给上游客户端
