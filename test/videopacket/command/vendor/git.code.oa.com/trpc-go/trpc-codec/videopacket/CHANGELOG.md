# Change Log

## [0.1.11](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.11) (2021-08-02)
### Bug Fixes
- client decode 错误区分，返回业务错误

## [0.1.10](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.10) (2021-07-23)
### Bug Fixes
- 去除多余的包头设置

## [0.1.9](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.9) (2021-04-28)
### Features
- 拆分风险函数，降低圈复杂度，使之符合EPC要求

## [0.1.8](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.8) (2021-04-26)
### Bug Fixes
- fix trpc4videopacket 工具对于jce 定义多个 interface 解析出错的 bug
- fix 依赖的 git.code.oa.com/trpc-go/trpc-utils/copyutils interface 定义变化不兼容的bug #68
- fix bodypack 的序列化类型为 8001 避免与其他业务冲突

## [0.1.7](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.7) (2021-04-13)
### Features
- 支持命令字为 0 的命令字路由模式，需要显示调用函数 ForceEnableCommand

## [0.1.6](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.6) (2021-02-03)
### Features
- trpc4videopacket 生成工具生成的 struct 默认带上 xml 标签
- 兼容命令字过小的问题 #50

## [0.1.5](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.5) (2021-01-04)
### Features
- 兼容 bodypack 的方式

### Bug Fixes
- 客户端编码中header未设置ToServerName时, 不覆盖msg.CalleeServer, 007主调可以正常上报

## [0.1.4](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.4) (2020-11-16)
### Bug Fixes
- 修复 --bug=83356249 codec videopacket 对于命令字 0x0128 路由错误的问题

## [0.1.3](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.3) (2020-10-12)
### Bug Fixes
- 解决服务端设置返回包头的问题

## [0.1.2](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.2) (2020-08-06)
### Features
- 服务段解码中的header msg 设置 callerapp calllerserver 等字段
- 增加最大包 2M 的限制

## [0.1.1](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.1) (2020-05-11)
### Features
- 生成的客户端代码增加 CalleeMethod 信息供天机阁使用

## [0.1.0](https://git.code.oa.com/trpc-go/trpc-codec/tree/videopacket/v0.1.0) (2020-01-12)

### Features
- 支持提供 videopacket 协议的服务
- 支持调用 videopacket 协议的后台
- 支持将 videopacket 协议的命令字改造成rpc模式

