# tRPC-Go framework

[![BK Pipelines Status](https://api.bkdevops.qq.com/process/api/external/pipelines/projects/pcgtrpcproject/p-da0d17b2016f404fa725983ae020ed01/badge?X-DEVOPS-PROJECT-ID=pcgtrpcproject)](http://api.devops.oa.com/process/api-html/user/builds/projects/pcgtrpcproject/pipelines/p-da0d17b2016f404fa725983ae020ed01/latestFinished?X-DEVOPS-PROJECT-ID=pcgtrpcproject) [![Coverage](http://covertest.oa.com/covtest/outerInte/getTotalImg/?pipeline_id=p-da0d17b2016f404fa725983ae020ed01)](http://covertest.oa.com/covtest/outerInte/getTotalLink/?pipeline_id=p-da0d17b2016f404fa725983ae020ed01) [![Benchmark](http://rdsopenapi.apigw.o.oa.com/prod/ciserver/get_eab_data?app_code=trpc-benchmark&amp;app_secret=ACYyoygRWOx7IsyZARwRu3hTDlRHzxjlcToCR7bWogMA3fRi3g&amp;frameName=tRPC-Go&amp;pressTestInstance=echo&amp;frameVersion=v0.3.5)](http://show.wsd.com/show3.htm?viewId=t_md_platform_server_eab_pressuretest_data) [![GoDoc](https://img.shields.io/badge/API%20Docs-GoDoc-green)](http://godoc.oa.com/git.code.oa.com/trpc-go/trpc-go)  [![iwiki](https://img.shields.io/badge/Wiki-iwiki-green)](https://iwiki.oa.tencent.com/display/tRPC/tRPC-Go)


tRPC-Go框架是公司统一微服务框架的golang版本，主要是以高性能，可插拔，易测试为出发点而设计的rpc框架。

# 文档地址：[iwiki](https://iwiki.oa.tencent.com/display/tRPC/tRPC-Go)
# 需求管理：[tapd](http://tapd.oa.com/trpc_go/prong/stories/stories_list)


## TRY IT !!!

# 整体架构
![架构图](https://git.code.oa.com/trpc-go/trpc-go/uploads/76DF446E40304476B8E12903E78B5EC4/2FE60489777F72A5901D36F114CFF331.png)

- 一个server进程内支持启动多个service服务，监听多个地址。
- 所有部件全都可插拔，内置transport等基本功能默认实现，可替换，其他组件需由第三方业务自己实现并注册到框架中。
- 所有接口全都可mock，使用gomock&mockgen生成mock代码，gostub为函数打桩，方便测试。
- 支持任意的第三方业务协议，只需实现业务协议打解包接口即可。默认支持trpc和http协议，随时切换，无差别开发cgi与后台server。
- 提供生成代码模板的trpc命令行工具。

# 插件管理
- 框架插件化管理设计只提供标准接口及接口注册能力。
- 外部组件由第三方业务作为桥梁把系统组件按框架接口包装起来，并注册到框架中。
- 业务使用时，只需要import包装桥梁路径。
- 具体插件原理可参考[plugin](blob/master/plugin) 。

# 生成工具
- 安装

```
git clone http://git.code.oa.com/trpc-go/trpc-go-cmdline.git  // or git clone git@git.code.oa.com:trpc-go/trpc-go-cmdline.git
cd trpc-go-cmdline
make && make install  //有更新时需要先卸载： make uninstall

```

- 使用
```bash
trpc help create
```
```bash
指定pb文件快速创建工程或rpcstub，

'trpc create' 有两种模式:
- 生成一个完整的服务工程
- 生成被调服务的rpcstub，需指定'-rpconly'选项.

Usage:
  trpc create [flags]

Flags:
      --alias                  enable alias mode of rpc name
      --assetdir string        path of project template
  -f, --force                  enable overwritten existed code forcibly
  -h, --help                   help for create
      --lang string            programming language, including go, java, python (default "go")
  -m, --mod string             go module, default: ${pb.package}
  -o, --output string          output directory
      --protocol string        protocol to use, trpc, http, etc (default "trpc")
      --protodir stringArray   include path of the target protofile (default [.])
  -p, --protofile string       protofile used as IDL of target service
      --rpconly                generate rpc stub only
      --swagger                enable swagger to gen swagger api document.
  -v, --verbose                show verbose logging info
```

# 工程规范
- 1. tRPC-Go官方建议：
 - 1.1 golang使用go modules模式开发，go版本必须 1.11 以上
 - 1.2 服务描述协议采用protobuf格式，package分成三级 trpc.app.server 
 - 1.3 app通常是一个业务项目，以一个group为粒度，每个服务server以一个repo为粒度放在具体的app下面来组织管理服务结构，每个服务独立仓库，CI友好
 - 1.4 pb.go协议生成文件由git统一协议管理平台来管理，所有协议在一个大的group下面，如trpcprotocol，每个app一个仓库，一个server协议一个子目录，协议与服务分离，接口联调，语言无关
 - 1.5 每个协议文件都必须指定 option go_package="git.code.oa.com/trpcprotocol/app/server"; 指明协议的git存放地址
- 2. 新建服务时，用户自己新建repo，在repo根目录下写proto文件，执行 go mod init git.code.oa.com/app/server 
- 3. 根目录下执行 trpc create --protofile=xxx.proto 命令在本地生成pb.go文件以及项目工程模板
- 4. 自测阶段可以先使用go mod的replace功能暂时替换远程统一协议仓库git路径并指向本地pb路径：
 - 4.1 首先需要在本地的pb.go目录下生成go mod文件，执行 go mod init git.code.oa.com/app/server/server
 - 4.2 替换自己的远程协议路径 go mod edit -replace=git.code.oa.com/trpcprotocol/app/server=./stub
 - 4.3 开发完成后将稳定的pb.go文件push到远程统一协议管理仓库git.code.oa.com/trpcprotocol/app/server，供调用方直接import使用
- 5. 根目录下执行 go build 生成服务程序
- 6. 根目录下执行单元测试 go test -v

# 服务协议
- trpc框架支持任意的第三方协议，同时默认支持了trpc和http协议
- 只需在配置文件里面指定protocol字段等于http即可启动一个cgi服务
- 使用同样的服务描述协议，完全一模一样的代码，可以随时切换trpc和http，达到真正意义上无差别开发cgi和后台服务的效果
- http访问链接默认为 http://xxx.com/package.service/method ，如有使用@alias=xxx替换则为 http://xxx.com/xxx 
- 请求数据使用http post方法携带，并解析到method里面的request结构体，通过http header content-type(application/json or application/pb)指定使用pb还是json 
- 第三方自定义业务协议可以参考[codec](blob/master/codec)

# mock测试
- 对于开发人员来说，最常见的问题是当你开发功能逻辑时，下游依赖服务还不可用，这个时候就可以使用gomock了。
- 对于测试人员来说，最常见的需求是写测试用例期望特定输出，故障注入，延时重试等，这个时候可以使用 testify gomock 。
- 以mock client proxy为例，每个pb文件通过trpc工具都会默认生成client proxy的mock代码：
``` 

// 开始写mock逻辑
ctrl := gomock.NewController(t)  
defer ctrl.Finish()

mockcli := pb.NewMockGreeterClientProxy(ctrl)  // pb自动生成的mock接口

// 预期行为
mockcli.EXPECT().SayHello(context.Background(), request).Return(response, nil)

// 为mock打桩分成两种：
// 1 client proxy为全局变量或者外部参数，这个时候可以直接将变量替换成mockcli即可
// 2 client proxy为内部函数创建的，这个时候需要使用gostub打桩
stubs := gostub.Stub(&pb.NewGreeterClientProxy, func(opts ...client.Option) pb.GreeterClientProxy { return mockcli })
defer stubs.Reset()

// 开始执行自己函数的测试用例

```
 
# 相关文档
- [框架设计文档](https://iwiki.oa.tencent.com/display/tRPC/tRPC-Go)
- [trpc工具详细说明](https://git.code.oa.com/trpc-go/trpc-go-cmdline)
- [helloworld开发指南](https://git.code.oa.com/trpc-go/trpc-go/tree/master/examples/helloworld)
- [第三方插件cl5实现demo](https://git.code.oa.com/trpc-go/trpc-selector-cl5)
- [第三方协议实现demo](https://git.code.oa.com/trpc-go/trpc-codec)

# 如何贡献
tRPC-Go项目组有专门的[tapd需求管理](http://tapd.oa.com/trpc_go/prong/stories/stories_list)，里面包括了各个具体功能点以及负责人和排期时间，
有兴趣的同学可以看看里面 <font color=#DC143C>需求状态为规划中</font> 的功能，自己认领任务，一起为tRPC-Go做贡献。
认领时将状态流转为： 需求已确认
开始投入将状态流转为： 开发中
开发完成将状态流转为： 已发布
开发中 和 已发布 之间时间不要超过两周。需求比较大的单可以拆分成多个子需求。
