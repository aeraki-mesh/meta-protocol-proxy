package client

import (
	"git.code.oa.com/trpc-go/trpc-go/config"
)

// BackendConfig 后端配置参数, 框架提供替换后端配置参数能力，可以由第三方注册进来，默认为空
type BackendConfig struct {
	// Callee 对端服务协议文件的callee service name
	// 配置文件以这个为key来设置参数, 一般callee和下面这个servicename是一致的
	// 可以为空，也可以支持用户随便自定义下游service name
	Callee      string `yaml:"callee"`
	ServiceName string `yaml:"name"` // 对端服务真实名字服务的service name
	Namespace   string // 对端服务环境 正式环境 测试环境

	Target   string // 默认使用北极星，一般不用配置，cl5://sid
	Password string

	Discovery      string
	Loadbalance    string
	Circuitbreaker string

	Network  string // tcp udp
	Timeout  int    // 单位 ms
	Protocol string // trpc

	Serialization *int // 序列化方式,因为默认值0已经用于pb了，所以通过指针来判断是否配置
	Compression   int  // 压缩方式

	TLSKey        string `yaml:"tls_key"`         // client秘钥
	TLSCert       string `yaml:"tls_cert"`        // client证书
	CACert        string `yaml:"ca_cert"`         // ca证书，用于校验server证书，调用tls服务，如https server
	TLSServerName string `yaml:"tls_server_name"` // client校验server服务名，调用https时，默认为hostname

	Filter []string
}

var defaultBackendConf = &BackendConfig{
	Network:  "tcp",
	Protocol: "trpc",
}

// DefaultClientConfig 后端调用配置信息，由业务配置解析并赋值更新，框架读取该结构
var DefaultClientConfig = make(map[string]*BackendConfig) // client proto service name => client backend config

// LoadClientConfig 通过本地配置文件路径解析业务配置并注册到框架中
func LoadClientConfig(path string, opts ...config.LoadOption) error {

	conf, err := config.DefaultConfigLoader.Load(path, opts...)
	if err != nil {
		return err
	}
	tmp := make(map[string]*BackendConfig)
	err = conf.Unmarshal(tmp)
	if err != nil {
		return err
	}
	RegisterConfig(tmp)
	return nil
}

// InsertFilters 在已有拦截器前面插入拦截器，并去重
func (c *BackendConfig) InsertFilters(filter []string) {
	filters := make([]string, 0, len(filter)+len(c.Filter))

	// 对filter配置进行去重
	fm := make(map[string]bool)
	for _, f := range append(filter, c.Filter...) {
		if _, ok := fm[f]; ok {
			continue
		}
		filters = append(filters, f)
		fm[f] = true
	}

	c.Filter = filters
}

// Config 通过对端协议文件的callee service name获取后端配置
func Config(serviceName string) *BackendConfig {
	if len(DefaultClientConfig) == 0 {
		return defaultBackendConf
	}
	conf, ok := DefaultClientConfig[serviceName]
	if !ok {
		conf, ok = DefaultClientConfig["*"]
		if !ok {
			return defaultBackendConf
		}
	}
	return conf
}

// RegisterConfig 业务自己解析完配置后 全局替换注册后端配置信息
func RegisterConfig(conf map[string]*BackendConfig) {
	DefaultClientConfig = conf
}

// RegisterClientConfig 业务自己解析完配置后 注册单个后端配置信息 不可并发调用
func RegisterClientConfig(calleeServiceName string, conf *BackendConfig) {
	DefaultClientConfig[calleeServiceName] = conf
}
