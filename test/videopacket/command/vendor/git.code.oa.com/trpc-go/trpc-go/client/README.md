# tRPC-Go 后端调用

## 客户端调用模式

```golang
proxy := pb.NewGreeterClientProxy()
rsp, err := proxy.SayHello(ctx, req)
if err != nil {
	log.Errorf("say hello fail:%v", err)
	return err
}
return nil
```

## 相关概念解析
- proxy 客户端调用桩函数或者调用代理，由trpc工具自动生成，内部调用client，proxy是个很轻量的结构，内部不会创建链接，每次请求每次生成即可
- target 后端服务的地址，规则为 selectorname://endpoint ，默认使用北极星+servicename，一般不需要设置，用于自测或者兼容老寻址方式如l5 cmlb ons等
- config 后端服务的配置，框架提供client.RegisterConfig注册配置能力，这样每次调用后端就可以自动从配置读取后端访问参数，不需要用户自己设置，需要业务自己解析配置文件并注册进来，默认以被调方协议文件的servicename(package.service)为key获取相关配置
- WithReqHead 一般情况，用户不需要关心协议头，都在框架底层做了，但在跨协议调用时就需要用户自己设置请求包头
- WithRspHead 设置响应包头承载结构，回传协议响应头，一般用于接入转发层

## 后端配置管理
- 后端配置一般与环境相关，每个环境都有自己的独立配置，包括下游的rpc地址和数据库db地址
- 应该使用远程配置中心，配置更新会自动生效，无需重启，也可以自己灰度控制，可参考[tconf](https://git.code.oa.com/trpc-go/trpc-config-tconf)或者[rainbow](https://git.code.oa.com/trpc-go/trpc-config-rainbow)
- 没有配置中心的情况下才使用trpc_go.yaml里面的client配置区块，常用于自测过程
远程配置样例如下：
```yaml
client:                                            #客户端调用的后端配置
  timeout: 1000                                    #针对所有后端的请求最长处理时间
  namespace: Development                           #针对所有后端的环境
  filter:                                          #针对所有后端的拦截器配置数组
    - m007                                         #所有后端接口请求都上报007监控
  service:                                         #针对单个后端的配置
    - callee: trpc.test.helloworld.Greeter         #后端服务协议文件的service name, 如果callee和下面的name一样，那只需要配置一个即可
      name: trpc.test.helloworld.Greeter1          #后端服务名字路由的service name，有注册到名字服务的话，下面target不用配置
      target: ip://127.0.0.1:8000                  #后端服务地址, ip://ip:port polaris://servicename cl5://sid cmlb://appid ons://zkname
      network: tcp                                 #后端服务的网络类型 tcp udp
      protocol: trpc                               #应用层协议 trpc http
      timeout: 800                                 #当前这个请求最长处理时间
      serialization: 0                             #序列化方式 0-pb 1-jce 2-json 3-flatbuffer，默认不要配置
      compression: 1                               #压缩方式 1-gzip 2-snappy 3-zlib，默认不要配置
      filter:                                      #针对单个后端的拦截器配置数组
        - tjg                                      #只有当前这个后端上报tjg
```

## 客户端调用流程
- 1. 设置相关配置参数
- 2. 通过target解析selector，为空则使用框架自己的寻址流程
- 3. 通过selector获取节点信息
- 4. 通过节点信息加载节点配置
- 5. 调用前置拦截器
- 6. 序列化body
- 7. 压缩body
- 8. 打包整个请求
- 9. 调用网络请求
- 10. 解包整个响应
- 11. 解压body
- 12. 反序列化body
- 13. 调用后置拦截器
