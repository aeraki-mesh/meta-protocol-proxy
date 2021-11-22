package client

import (
	"fmt"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/filter"
	"git.code.oa.com/trpc-go/trpc-go/naming/circuitbreaker"
	"git.code.oa.com/trpc-go/trpc-go/naming/discovery"
	"git.code.oa.com/trpc-go/trpc-go/naming/loadbalance"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
	"git.code.oa.com/trpc-go/trpc-go/naming/selector"
	"git.code.oa.com/trpc-go/trpc-go/pool/connpool"
	"git.code.oa.com/trpc-go/trpc-go/transport"
)

// Options 客户端调用参数
type Options struct {
	ServiceName       string        // 后端服务service name
	CallerServiceName string        // 调用服务service name 即server自身服务名
	CalleeMethod      string        // 用于监控上报的method方法名
	Timeout           time.Duration // 后端调用超时时间

	Target   string // 后端服务地址 name://endpoint 兼容老寻址方式 如 cl5://sid cmlb://appid ip://ip:port
	endpoint string // 默认等于 service name，除非有指定target

	Network     string
	CallOptions []transport.RoundTripOption // client transport需要调用的参数
	Transport   transport.ClientTransport

	SelectOptions []selector.Option
	Selector      selector.Selector

	CurrentSerializationType int
	CurrentCompressType      int
	SerializationType        int
	CompressType             int

	Codec    codec.Codec
	MetaData codec.MetaData

	Filters       filter.Chain // 链式拦截器
	DisableFilter bool         // 是否禁用拦截器

	ReqHead interface{}    // 提供用户设置自定义请求头的能力
	RspHead interface{}    // 提供用户获取自定义响应头的能力
	Node    *registry.Node // 提供用户获取具体请求节点的能力
}

// Option 调用参数工具函数
type Option func(*Options)

// WithNamespace 设置 namespace 后端服务环境 正式环境 Production 测试环境 Development
func WithNamespace(namespace string) Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithNamespace(namespace))
	}
}

// WithServiceName 设置后端服务service name
func WithServiceName(s string) Option {

	return func(o *Options) {
		o.ServiceName = s
		o.endpoint = s
	}
}

// WithCallerServiceName 设置主调服务service name, 即自身服务的service name
func WithCallerServiceName(s string) Option {

	return func(o *Options) {
		o.CallerServiceName = s
		o.SelectOptions = append(o.SelectOptions, selector.WithSourceServiceName(s))
	}
}

// WithCallerNamespace 设置主调服namespace, 即自身服务 namespace
func WithCallerNamespace(s string) Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithSourceNamespace(s))
	}
}

// WithDisableFilter 禁用拦截器, 假如插件里面setup时需要使用client，但这时候filter都还没初始化，此时就可以先禁用拦截器
func WithDisableFilter() Option {

	return func(o *Options) {
		o.DisableFilter = true
	}
}

// WithDisableServiceRouter 禁用服务路由
func WithDisableServiceRouter() Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithDisableServiceRouter())
	}
}

// WithEnvKey 设置环境key
func WithEnvKey(key string) Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithEnvKey(key))
	}
}

// WithCallerEnvName 设置当前环境
func WithCallerEnvName(envName string) Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithSourceEnvName(envName))
	}
}

// WithCallerSetName 设置调用者set分组
func WithCallerSetName(setName string) Option {
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithSourceSetName(setName))
	}
}

// WithCalleeSetName 指定set分组调用
func WithCalleeSetName(setName string) Option {
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithDestinationSetName(setName))
	}
}

// WithCalleeEnvName 设置被调服务环境
func WithCalleeEnvName(envName string) Option {

	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithDestinationEnvName(envName))
	}
}

// WithCalleeMethod 指定下游方法名
func WithCalleeMethod(method string) Option {
	return func(o *Options) {
		o.CalleeMethod = method
	}
}

// WithCallerMetadata 增加主调服务的路由匹配元数据，env/set路由请使用相应配置函数
func WithCallerMetadata(key string, val string) Option {
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithSourceMetadata(key, val))
	}
}

// WithCalleeMetadata 增加被调服务的路由匹配元数据，env/set路由请使用相应配置函数
func WithCalleeMetadata(key string, val string) Option {
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithDestinationMetadata(key, val))
	}
}

// WithBalancerName 通过名字指定负载均衡
func WithBalancerName(balancerName string) Option {

	balancer := loadbalance.Get(balancerName)
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithLoadBalance(balancer))
	}
}

// WithDiscoveryName 通过名字指定名字服务
func WithDiscoveryName(name string) Option {

	d := discovery.Get(name)
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithDiscovery(d))
	}
}

// WithCircuitBreakerName 通过名字指定熔断器
func WithCircuitBreakerName(name string) Option {

	cb := circuitbreaker.Get(name)
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithCircuitBreaker(cb))
	}
}

// WithKey 设置有状态的路由key
func WithKey(key string) Option {
	return func(o *Options) {
		o.SelectOptions = append(o.SelectOptions, selector.WithKey(key))
	}
}

// WithTarget 调用目标地址schema name://endpoint 如 cl5://sid ons://zkname ip://ip:port
func WithTarget(t string) Option {

	return func(o *Options) {
		o.Target = t
	}
}

// WithNetwork 对端服务网络类型 tcp or udp, 默认tcp
func WithNetwork(s string) Option {

	return func(o *Options) {
		o.Network = s
		o.CallOptions = append(o.CallOptions, transport.WithDialNetwork(s))
	}
}

// WithPassword 对端服务请求密码
func WithPassword(s string) Option {

	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithDialPassword(s))
	}
}

// WithPool 请求后端时 自定义tcp连接池
func WithPool(pool connpool.Pool) Option {

	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithDialPool(pool))
	}
}

// WithTimeout 请求后端超时时间 默认1s
func WithTimeout(t time.Duration) Option {

	return func(o *Options) {
		o.Timeout = t
	}
}

// WithCurrentSerializationType 设置当前请求序列化方式，指定后端协议内部序列化方式，使用 WithSerializationType
func WithCurrentSerializationType(t int) Option {

	return func(o *Options) {
		o.CurrentSerializationType = t
	}
}

// WithSerializationType 指定后端协议内部序列化方式，一般只需指定该option，current用于代理转发层
func WithSerializationType(t int) Option {

	return func(o *Options) {
		o.SerializationType = t
	}
}

// WithCurrentCompressType 设置当前请求解压缩方式，指定后端协议内部解压缩方式，使用 WithCompressType
func WithCurrentCompressType(t int) Option {

	return func(o *Options) {
		o.CurrentCompressType = t
	}
}

// WithCompressType 指定后端协议内部解压缩方式，一般只需指定该option，current用于代理转发层
func WithCompressType(t int) Option {

	return func(o *Options) {
		o.CompressType = t
	}
}

// WithTransport 替换底层client通信层
func WithTransport(t transport.ClientTransport) Option {

	return func(o *Options) {
		o.Transport = t
	}
}

// WithProtocol 指定后端服务协议名字 如 trpc
func WithProtocol(s string) Option {

	return func(o *Options) {
		o.Codec = codec.GetClient(s)

		r := transport.GetFramerBuilder(s)
		if r != nil {
			o.CallOptions = append(o.CallOptions, transport.WithClientFramerBuilder(r))
		}

		trans := transport.GetClientTransport(s)
		if trans != nil {
			o.Transport = trans
		}
	}
}

// WithConnectionMode 设置连接是否为connected模式（connected限制udp只收相同路径回包）
func WithConnectionMode(connMode transport.ConnectionMode) Option {

	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithConnectionMode(connMode))
	}
}

// WithSendOnly 设置只发不收，一般用于udp异步发送
func WithSendOnly() Option {

	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithReqType(transport.SendOnly))
	}
}

// WithFilter 添加客户端拦截器，支持在 打包前 解包后 拦截处理
func WithFilter(fs filter.Filter) Option {

	return func(o *Options) {
		o.Filters = append(o.Filters, fs)
	}
}

// WithFilters 添加客户端拦截器，支持在 打包前 打包后 拦截处理
func WithFilters(fs []filter.Filter) Option {
	return func(o *Options) {
		o.Filters = append(o.Filters, fs...)
	}
}

// WithReqHead 设置后端请求包头，可不设置，默认会从请求源头clone server req head
func WithReqHead(h interface{}) Option {

	return func(o *Options) {
		o.ReqHead = h
	}
}

// WithRspHead 设置后端响应包头，不关心时可不设置, 一般用于网关服务
func WithRspHead(h interface{}) Option {

	return func(o *Options) {
		o.RspHead = h
	}
}

// WithMetaData 设置透传参数
func WithMetaData(key string, val []byte) Option {

	return func(o *Options) {
		if o.MetaData == nil {
			o.MetaData = codec.MetaData{}
		}
		o.MetaData[key] = val
	}
}

// WithSelectorNode 设置后端selector寻址node结果保存器，不关心时可不设置, 常用于定位问题节点
func WithSelectorNode(n *registry.Node) Option {

	return func(o *Options) {
		o.Node = n
	}
}

// setNamingOptions 设置寻址相关 option
func (opts *Options) setNamingOptions(cfg *BackendConfig) error {
	if cfg.ServiceName != "" {
		opts.ServiceName = cfg.ServiceName
		opts.endpoint = cfg.ServiceName
	}
	if cfg.Namespace != "" {
		opts.SelectOptions = append(opts.SelectOptions, selector.WithNamespace(cfg.Namespace))
	}

	if cfg.Target != "" {
		opts.Target = cfg.Target
		return nil
	}

	if cfg.Discovery != "" {
		d := discovery.Get(cfg.Discovery)
		if d == nil {
			return errs.NewFrameError(errs.RetServerSystemErr,
				fmt.Sprintf("client config: discovery %s no registered", cfg.Discovery))
		}
		opts.SelectOptions = append(opts.SelectOptions, selector.WithDiscovery(d))
	}
	if cfg.Loadbalance != "" {
		balancer := loadbalance.Get(cfg.Loadbalance)
		if balancer == nil {
			return errs.NewFrameError(errs.RetServerSystemErr,
				fmt.Sprintf("client config: balancer %s no registered", cfg.Loadbalance))
		}
		opts.SelectOptions = append(opts.SelectOptions, selector.WithLoadBalance(balancer))
	}
	if cfg.Circuitbreaker != "" {
		cb := circuitbreaker.Get(cfg.Circuitbreaker)
		if cb == nil {
			return errs.NewFrameError(errs.RetServerSystemErr,
				fmt.Sprintf("client config: circuitbreaker %s no registered", cfg.Circuitbreaker))
		}
		opts.SelectOptions = append(opts.SelectOptions, selector.WithCircuitBreaker(cb))
	}

	return nil
}

// LoadClientConfig 通过key读取后端配置, key默认为proto协议文件里面的callee service name
func (opts *Options) LoadClientConfig(key string) error {

	cfg := Config(key)

	opts.setNamingOptions(cfg)

	if cfg.Timeout > 0 {
		opts.Timeout = time.Duration(cfg.Timeout) * time.Millisecond
	}
	if cfg.Serialization != nil {
		opts.SerializationType = *cfg.Serialization
	}
	if cfg.Compression > 0 {
		opts.CompressType = cfg.Compression
	}

	if cfg.Protocol != "" {
		o := WithProtocol(cfg.Protocol)
		o(opts)
	}
	if cfg.Network != "" {
		opts.Network = cfg.Network
		opts.CallOptions = append(opts.CallOptions, transport.WithDialNetwork(cfg.Network))
	}
	if cfg.Password != "" {
		opts.CallOptions = append(opts.CallOptions, transport.WithDialPassword(cfg.Password))
	}
	if cfg.CACert != "" {
		opts.CallOptions = append(opts.CallOptions,
			transport.WithDialTLS(cfg.TLSCert, cfg.TLSKey, cfg.CACert, cfg.TLSServerName))
	}

	return nil
}

// LoadClientFilterConfig 通过key读取后端Filter配置
func (opts *Options) LoadClientFilterConfig(key string) error {

	cfg := Config(key)

	if !opts.DisableFilter {
		for _, filterName := range cfg.Filter {
			filt := filter.GetClient(filterName)
			if filt == nil {
				return errs.NewFrameError(errs.RetServerSystemErr,
					fmt.Sprintf("client config: filter %s no registered", filterName))
			}
			opts.Filters = append(opts.Filters, filt)
		}
	}

	return nil
}

// LoadNodeConfig 通过注册中心返回的节点信息设置参数
func (opts *Options) LoadNodeConfig(node *registry.Node) {

	opts.CallOptions = append(opts.CallOptions, transport.WithDialAddress(node.Address))

	if node.Network != "" {
		opts.Network = node.Network
		opts.CallOptions = append(opts.CallOptions, transport.WithDialNetwork(node.Network))
	}

	if node.Protocol != "" {
		o := WithProtocol(node.Protocol)
		o(opts)
	}
}

// WithTLS 指定client tls文件地址, caFile CA证书，用于校验server证书, 一般调用https只需要指定caFile即可。
// 也可以传入caFile="none"表示不校验server证书, caFile="root"表示使用本机安装的ca证书来验证server。
// certFile客户端自身证书，keyFile客户端自身秘钥，服务端开启双向认证需要校验客户端证书时才需要发送客户端自身证书，一般为空即可。
// serverName客户端校验服务端的服务名，https可为空默认为hostname。
func WithTLS(certFile, keyFile, caFile, serverName string) Option {

	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithDialTLS(certFile, keyFile, caFile, serverName))
	}
}

// WithDisableConnectionPool 禁用连接池
func WithDisableConnectionPool() Option {
	return func(o *Options) {
		o.CallOptions = append(o.CallOptions, transport.WithDisableConnectionPool())
	}
}
