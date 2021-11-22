# Change Log

## [0.3.5](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.5) (2020-07-27)

### Bug Fixes
- 解决框架SetGlobalConfig后移导致插件启动失败问题
- 修复client namespace为空问题

## [0.3.4](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.4) (2020-07-24)

### Features
- rpc invalid时，添加当前服务service name，方便排查问题
- 提高单测覆盖率
- http端口443时默认设置schema为https
- 将开源lumberjack日志切换为内置rollwriter日志，提高打日志性能
- 解决圈复杂度问题，每个函数尽量控制到5以内
- 对端口复用的httpserver添加热重启时停止接收新请求

### Bug Fixes
- 解决动态设置日志等级无效问题
- 修复同一server使用多个证书时缓存冲突问题
- 修复http client 连接失败上报问题
- 解决server write 错误导致死循环问题
- 解决server代理透传二进制问题
- 解决http get请求无法解析二进制字段问题
- 解决框架启动调用两次SetGlobalConfig问题

## [0.3.3](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.3) (2020-07-01)

### Features
- http default transport使用原生标准库的default transport
- 支持client短连接模式
- 支持设置自定义连接池
- 日志key字段支持配置
- 连接池MaxIdle最大连接数调整为无上限

### Bug Fixes
- 解决server filter去重问题
- 解决ip硬编码安全规范问题
- 解决代码规范问题

## [0.3.2](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.2) (2020-06-18)

### Features
- 支持server端异步处理请求，解决非trpc-go client调用超时问题
- 框架内部默认import uber automaxprocs，解决容器内调度延迟问题

### Bug Fixes
- 解决client filter覆盖清空问题
- 解决http server CRLF注入问题

## [0.3.1](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.1) (2020-06-10)

### Features
- 支持用户自己设置Listener
- 支持http get请求独立序列化方式

### Bug Fixes
- 解决client filter执行两次的问题 
- 解决server回包无法指定序列化方式和压缩方式问题
- 解决http client proxy用户无法设置protocol的问题

## [0.3.0](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.3.0) (2020-05-29)

### Features
- 支持传输层tls鉴权
- 支持http2 protocol
- 支持admin动态设置不同logger不同output的日志等级
- 支持http Put Delete方法

## [0.2.8](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.8) (2020-05-12)

### Features
- 代码OWNER制度更改，owners.txt改成.code.yml，符合epc标准
- 支持http client post form请求
- 支持client SendOnly只发不收请求
- 支持自定义http路由mux
- 支持http.SetContentType设置http content-type到trpc serialization type的映射关系，兼容不规范老http框架服务返回乱写的content-type

### Bug Fixes
- 解决http client rsp没有反序列化问题
- 解决tcp server空闲时间不生效问题
- 解决多次调用log.WithContextFields新增字段不生效问题

## [0.2.7](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.7) (2020-04-30)

### Bug Fixes
- 解决flag启动失败问题

## [0.2.6](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.6) (2020-04-29)

### Features
- 复用msg结构体，echo服务性能从39w/s提升至41w/s
- 提升单元测试覆盖率至84.6%
- 新增一致性哈希路由算法

### Bug Fixes
- tcp listener没有close
- 解决NewServer flag定义冲突问题

## [0.2.5](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.5) (2020-04-20)

### Features
- 添加trpc.NewServerWithConfig允许用户自定义框架配置文件格式
- 支持https client，支持https双向认证
- 支持http mock
- 添加性能数据实时看板，readme benchmark icon入口

### Bug Fixes
- 将所有gogo protobuf改成官方的golang protobuf，解决兼容问题
- admin启动失败直接panic，解决admin启动失败无感知问题

## [0.2.4](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.4) (2020-04-02)

### Features
- http server head 添加原始包体ReqBody
- 配置文件支持toml序列化方式
- 添加client CalleeMethod option，方便自定义监控方法名
- 添加dns寻址方式：dns://domain:port

### Bug Fixes
- 改造log api，将Warning改成Warn
- 更改DefaultSelector为接口方式

## [0.2.3](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.3) (2020-03-24)

### Bug Fixes
- 禁用client filter时不加载filter配置

## [0.2.2](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.2) (2020-03-23)

### Features
- 框架内部关键错误上报metrics
- 多维监控使用数组形式

## [0.2.1](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.1) (2020-03-19)

### Features
- 支持禁用client拦截器

## [0.2.0](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.2.0) (2020-03-18)

### Bug Fixes
- 解决golint问题

### Features
- 支持set路由
- client config支持配置下游的序列化方式和压缩方式
- 框架支持metrics标准多维监控接口
- 所有wiki文档全部转移到iwiki

## [0.1.6](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.6) (2020-03-11)

### Bug Fixes
- 新增插件初始化完成事件通知

## [0.1.5](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.5) (2020-03-09)

### Bug Fixes
- 解决golint问题
- 解决client transport收包失败都返回101超时错误码问题

### Features
- client transport framer复用
- http server decode失败返回400，encode失败返回500
- 新增更安全的多并发简易接口 trpc.GoAndWait 
- 新增http client通用的Post Get方法
- server拦截器未注册不让启动
- 日志caller skip支持配置
- 支持https server
- 添加上游客户端主动断开连接，提前取消请求错误码 161

## [0.1.4](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.4) (2020-02-18)

### Bug Fixes
- 客户端设置不自动解压缩失效问题

## [0.1.3](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.3) (2020-02-13)

### Bug Fixes
- 插件初始化加载bug

## [0.1.2](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.2) (2020-02-12)

### Bug Fixes
- http client codec CalleeMethod覆盖问题
- server/client mock api失效问题

### Features
- 新增go1.13错误处理error wrapper模式
- 添加插件初始化依赖顺序逻辑
- 新增trpc.BackgroundContext()默认携带环境信息，避免用户使用错误

## [0.1.1](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.1) (2020-01-21)

### Bug Fixes
- http client transport无法设置content-type问题
- 天机阁ClientFilter取不到CalleeMethod问题
- http client transport无法设置host问题

### Features
- 增加disable_request_timeout配置开关，允许用户自己决定是否继承上游超时时间，默认会继承
- 增加callee framework error type，用以区分当前框架错误码，下游框架错误码，业务错误码
- 下游超时时，errmsg自动添加耗时时间，方便定位问题
- http server回包header增加nosniff安全header
- http被调method使用url上报


## [0.1.0](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0) (2020-01-10)

### Bug Fixes
- 滚动日志默认按大小，流水日志按日期
- 日志路径和文件名拼接bug
- 指定环境名路由bug

### Features
- 代码格式优化，符合epc标准
- 插件上报统计数据

## [0.1.0-rc.14](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.14) (2020-01-06)

### Bug Fixes
- 连接池默认最大空闲连接数过小导致频繁创建fd，出现timewait爆满问题，改成默认MaxIdle=2048
- server transport没有framer builder导致请求crash问题

### Features
- 支持从名字服务获取被调方容器名

## [0.1.0-rc.13](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.13) (2019-12-30)

### Bug Fixes
- 连接池偶现EOF问题：server端统一空闲时间1min，client端统一空闲时间50s
- 高并发下超时设置http header crash问题：去除service select超时控制
- http回包json enum变字符串 改成 enum变数字，可配置
- http header透传信息二进制设置失败问题，改成transinfo base64编码

### Features
- 支持无协议文件自定义http路由
- 支持请求http后端携带header
- http服务支持reuseport热重启


## [0.1.0-rc.12](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.12) (2019-12-24)

### Bug Fixes
- 包大小uint16限制
- metrics counter锁bug
- 单个插件初始化超时3s，防止服务卡死
- 同名网卡ip覆盖
- 多logger失效

### Features
- 指定环境名路由
- http新增自定义ErrorHandler
- timer改成插件模式
- 添加godoc icon


## [0.1.0-rc.11](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.11) (2019-12-09)

### Bug Fixes
- udp client transport对象池复用导致buffer错乱


## [0.1.0-rc.10](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.10) (2019-12-05)

### Bug Fixes
- udp client connected模式writeto失败问题



## [0.1.0-rc.9](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.9) (2019-12-04)

### Bug Fixes
- 连接池超时控制无效
- 单测偶现失败
- 默认配置失效

### Features
- 新增多环境开关
- udp client transport新增connection mode，由用户自己控制请求模式
- udp收包使用对象池，优化性能
- admin新增性能分析接口


## [0.1.0-rc.8](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.8) (2019-11-26)

### Bug Fixes
- server WithProtocol option漏了transport
- 后端回包修改压缩方式不生效
- client namespace配置不生效

### Features
- 支持 client工具多环境路由
- 支持 admin管理命令
- 支持 热重启
- 优化 日志打印


## [0.1.0-rc.7](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.7) (2019-11-21)

### Features
- 支持client option设置多环境


## [0.1.0-rc.6](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.6) (2019-11-20)

### Bug Fixes
- 支持一致性哈希路由


## [0.1.0-rc.5](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.5) (2019-11-08)

### Bug Fixes
- tconf api
- transport空指针bug

### Features
- 多环境治理
- 代码质量管理owner机制


## [0.1.0-rc.4](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.4) (2019-11-04)

### Bug Fixes
- frame builder 魔数校验，最大包限制默认10M

### Features
- 提高单测覆盖率


## [0.1.0-rc.3](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.3) (2019-10-28)

### Bug Fixes
- http client codec

## [0.1.0-rc.2](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.2) (2019-10-25)

### Bug Fixes
- windows 连接池bug

### Features
- 测试覆盖率提高到83%

## [0.1.0-rc.1](https://git.code.oa.com/trpc-go/trpc-go/tree/v0.1.0-rc.1) (2019-10-25)

### Features
- 一发一收应答式服务模型
- 支持 tcp udp http 网络请求
- 支持 tcp连接池，buffer对象池
- 支持 server业务处理函数前后链式拦截器，client网络调用函数前后链式拦截器
- 提供trpc代码[生成工具](https://git.code.oa.com/trpc-go/trpc-go-cmdline)，通过 protobuf idl 生成工程服务代码模板
- 提供[rick统一协议管理平台](http://trpc.rick.oa.com/rick/pb/list)，tRPC-Go插件通过proto文件自动生成pb.go并自动push到[统一git](https://git.code.oa.com/trpcprotocol)
- 插件化支持 任意业务协议，目前已支持 trpc，[tars](https://git.code.oa.com/trpc-go/trpc-codec/tree/master/tars)，[oidb](https://git.code.oa.com/trpc-go/trpc-codec/tree/master/oidb)
- 插件化支持 任意序列化方式，目前已支持 protobuf，jce，json
- 插件化支持 任意压缩方式，目前已支持 gzip，snappy
- 插件化支持 任意链路跟踪系统，目前已使用拦截器方式支持 [天机阁](https://git.code.oa.com/trpc-go/trpc-opentracing-tjg) [jaeger](https://git.code.oa.com/trpc-go/trpc-opentracing-jaeger)
- 插件化支持 任意名字服务，目前已支持 [老l5](https://git.code.oa.com/trpc-go/trpc-selector-cl5)，[cmlb](https://git.code.oa.com/trpc-go/trpc-selector-cmlb)，[北极星测试环境](https://git.code.oa.com/trpc-go/trpc-naming-polaris) 
- 插件化支持 任意监控系统，目前已支持 [老sng-monitor-attr监控](https://git.code.oa.com/trpc-go/metrics-plugins/tree/master/attr)，[pcg 007监控](https://git.code.oa.com/trpc-go/metrics-plugins/tree/master/m007)
- 插件化支持 多输出日志组件，包括 终端console，本地文件file，[远程日志atta](https://git.code.oa.com/trpc-go/trpc-log-remote-atta)
- 插件化支持 任意负载均衡算法，目前已支持 roundrobin weightroundrobin
- 插件化支持 任意熔断器算法，目前已支持 北极星熔断器插件
- 插件化支持 任意配置中心系统，目前已支持 [tconf](https://git.code.oa.com/trpc-go/config-tconf)

### 压测报告

| 环境 | server | client | 数据 | tps | cpu |
| :--: | :--: |:--: |:--: |:--: |:--: |
| 1 | v8虚拟机 9.87.179.247 | 星海平台jmeter 9.21.148.88 | 10B的echo请求 | 25w/s | null |
| 2 | b70物理机 100.65.32.12 | 星海平台jmeter 9.21.148.88 | 10B的echo请求 | 42w/s | null |
| 3 | v8虚拟机 9.87.179.247 | eab工具，b70物理机 100.65.32.13 | 10B的echo请求 | 35w/s | 64% |
| 4 | b70物理机 100.65.32.12 | eab工具，b70物理机 100.65.32.13 | 10B的echo请求 | 60w/s | 45% |

### 测试报告
- 整体单元测试[覆盖率80%](http://devops.oa.com/console/pipeline/pcgtrpcproject/p-da0d17b2016f404fa725983ae020ed01/detail/b-5ee497f8d96348359b874ec062795ca5/output)
- 支持 [server mock能力](https://git.code.oa.com/trpc-go/trpc-go/tree/master/server/mockserver)
- 支持 [client mock能力](https://git.code.oa.com/trpc-go/trpc-go/tree/master/client/mockclient)

### 开发文档
- 每个package有[README.md](https://git.code.oa.com/trpc-go/trpc-go/tree/master/server)
- [examples/features](https://git.code.oa.com/trpc-go/trpc-go/tree/master/examples/features)有每个特性的代码示例
- [examples/helloworld](https://git.code.oa.com/trpc-go/trpc-go/tree/master/examples/helloworld)具体工程服务示例
- [git wiki](https://git.code.oa.com/trpc-go/trpc-go/wikis/home)有详细的设计文档，开发指南，FAQ等

### 下一版本功能规划
- 服务性能优化，提高tps
- 完善开发文档，提高易用性
- 完善单元测试，提高测试覆盖率
- 支持[更多协议](https://git.code.oa.com/trpc-go/trpc-codec)，打通全公司大部分存量平台框架
- admin命令行系统
- auth鉴权
- 多环境/set/idc/版本/哈希 路由能力
- 染色key能力
