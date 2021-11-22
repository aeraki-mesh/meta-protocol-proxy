// Package trpc tRPC-Go框架是公司统一微服务框架的golang版本，主要是以高性能，可插拔，易测试为出发点而设计的rpc框架
package trpc

import (
	"context"
	"flag"
	"fmt"
	"runtime"
	"sync"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/admin"
	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/filter"
	"git.code.oa.com/trpc-go/trpc-go/log"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
	"git.code.oa.com/trpc-go/trpc-go/server"

	_ "go.uber.org/automaxprocs" // 框架内部默认import，避免用户没有import导致容器出问题
)

// NewServer trpc框架通过配置文件快速启动支持多service的server, 全局只允许调用一次, 内部会自动调用flag.Parse解析参数不需要用户再调用flag
func NewServer(opt ...server.Option) *server.Server {
	// 默认为当前目录trpc_go.yaml，也可通过flag更改
	if ServerConfigPath == "./trpc_go.yaml" {
		flag.StringVar(&ServerConfigPath, "conf", "./trpc_go.yaml", "server config path")
		flag.Parse()
	}

	// 解析框架配置
	cfg, err := LoadConfig(ServerConfigPath)
	if err != nil {
		panic("parse config fail: " + err.Error())
	}
	// 保存到全局配置里面，方便其他插件获取配置数据
	SetGlobalConfig(cfg)

	// 加载插件
	err = Setup(cfg)
	if err != nil {
		panic("setup plugin fail: " + err.Error())
	}

	return NewServerWithConfig(cfg, opt...)
}

func startAdmin(cfg *Config) {
	// 有配置admin参数则启动admin服务
	if cfg.Server.Admin.Port == 0 {
		return
	}

	opts := []admin.Option{
		admin.WithVersion(Version()),
		admin.WithAddr(fmt.Sprintf("%s:%d", cfg.Server.Admin.IP, cfg.Server.Admin.Port)),
		admin.WithTLS(cfg.Server.Admin.EnableTLS),
		admin.WithConfigPath(ServerConfigPath),
	}

	if timeout := cfg.Server.Admin.ReadTimeout; timeout > 0 {
		opts = append(opts, admin.WithReadTimeout(time.Millisecond*time.Duration(timeout)))
	}

	if timeout := cfg.Server.Admin.WriteTimeout; timeout > 0 {
		opts = append(opts, admin.WithWriteTimeout(time.Millisecond*time.Duration(timeout)))
	}

	go func() {
		err := admin.Run(opts...)
		if err != nil {
			panic("run admin fail: " + err.Error())
		}
	}()
}

func newServiceWithConfig(cfg *Config, conf *ServiceConfig, opt ...server.Option) server.Service {
	var (
		timeout = time.Millisecond * time.Duration(conf.Timeout)
		filters = []filter.Filter{}
	)

	// 全局filter对所有service都生效，并且顺序靠前.
	filterNames := make([]string, 0, len(cfg.Server.Filter)+len(conf.Filter))

	// 对filter配置进行去重
	fm := make(map[string]bool)
	for _, name := range append(cfg.Server.Filter, conf.Filter...) {
		if _, ok := fm[name]; !ok {
			fm[name] = true
			filterNames = append(filterNames, name)
		}
	}

	for _, name := range filterNames {
		f := filter.GetServer(name)
		if f == nil {
			panic(fmt.Sprintf("filter %s no registered, do not configure", name))
		}
		filters = append(filters, f)
	}

	opts := []server.Option{
		server.WithNamespace(cfg.Global.Namespace),
		server.WithEnvName(cfg.Global.EnvName),
		server.WithServiceName(conf.Name),
		server.WithProtocol(conf.Protocol),
		server.WithNetwork(conf.Network),
		server.WithAddress(conf.Address),
		server.WithTimeout(timeout),
		server.WithFilters(filters),
		server.WithRegistry(registry.Get(conf.Name)),
		server.WithDisableRequestTimeout(conf.DisableRequestTimeout),
		server.WithTLS(conf.TLSCert, conf.TLSKey, conf.CACert),
		server.WithServerAsync(conf.ServerAsync),
	}
	if cfg.Global.EnableSet == "Y" {
		opts = append(opts, server.WithSetName(cfg.Global.FullSetName))
	}
	opts = append(opts, opt...)

	return server.New(opts...)
}

// NewServerWithConfig 使用自定义配置启动服务
func NewServerWithConfig(cfg *Config, opt ...server.Option) *server.Server {
	// 再设置一次，防止用户忘记设置
	SetGlobalConfig(cfg)

	s := &server.Server{}

	// 启动 admin
	startAdmin(cfg)

	// 逐个service加载配置并启动
	for _, conf := range cfg.Server.Service {
		s.AddService(conf.Name, newServiceWithConfig(cfg, conf, opt...))
	}

	return s
}

// ----------------------- trpc 通用工具类函数 ------------------------------------ //

// Message 从ctx获取请求通用数据
func Message(ctx context.Context) codec.Msg {
	return codec.Message(ctx)
}

// BackgroundContext 携带空msg的background context，常用于用户自己创建任务函数的场景
func BackgroundContext() context.Context {
	cfg := GlobalConfig()
	ctx, msg := codec.WithNewMessage(context.Background())
	msg.WithCalleeContainerName(cfg.Global.ContainerName)
	msg.WithNamespace(cfg.Global.Namespace)
	msg.WithEnvName(cfg.Global.EnvName)
	if cfg.Global.EnableSet == "Y" {
		msg.WithSetName(cfg.Global.FullSetName)
	}
	if len(cfg.Server.Service) > 0 {
		msg.WithCalleeServiceName(cfg.Server.Service[0].Name)
	} else {
		msg.WithCalleeApp(cfg.Server.App)
		msg.WithCalleeServer(cfg.Server.Server)
	}
	return ctx
}

// GoAndWait 封装更安全的多并发调用, 启动goroutine并等待所有处理流程完成，自动recover
// 返回值error返回的是多并发协程里面第一个返回的不为nil的error，主要用于关键路径判断，当多并发协程里面有一个是关键路径且有失败则返回err，其他非关键路径并发全部返回nil
func GoAndWait(handlers ...func() error) (err error) {
	var wg sync.WaitGroup
	var once sync.Once
	for _, f := range handlers {
		wg.Add(1)
		go func(handler func() error) {

			defer func() {
				if e := recover(); e != nil {
					buf := make([]byte, 1024)
					buf = buf[:runtime.Stack(buf, false)]
					log.Errorf("[PANIC]%v\n%s\n", e, buf)
				}
				wg.Done()
			}()

			if e := handler(); e != nil {
				once.Do(func() {
					err = e
				})
			}
		}(f)
	}

	wg.Wait()

	return err
}

// GetMetaData 从请求里面获取key的透传字段
func GetMetaData(ctx context.Context, key string) []byte {
	msg := codec.Message(ctx)
	if len(msg.ServerMetaData()) > 0 {
		return msg.ServerMetaData()[key]
	}
	return nil
}

// SetMetaData 设置透传字段返回给上游, 非并发安全
func SetMetaData(ctx context.Context, key string, val []byte) {
	msg := codec.Message(ctx)
	if len(msg.ServerMetaData()) > 0 {
		msg.ServerMetaData()[key] = val
		return
	}
	md := make(map[string][]byte)
	md[key] = val
	msg.WithServerMetaData(md)
}

// Request 获取trpc业务协议请求包头，不存在则返回空的包头结构体
func Request(ctx context.Context) *RequestProtocol {
	msg := codec.Message(ctx)
	request, ok := msg.ServerReqHead().(*RequestProtocol)
	if !ok {
		return &RequestProtocol{}
	}
	return request
}

// Response 获取trpc业务协议响应包头，不存在则返回空的包头结构体
func Response(ctx context.Context) *ResponseProtocol {
	msg := codec.Message(ctx)
	response, ok := msg.ServerRspHead().(*ResponseProtocol)
	if !ok {
		return &ResponseProtocol{}
	}
	return response
}
