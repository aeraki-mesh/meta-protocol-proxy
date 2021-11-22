package trpc

import (
	"io/ioutil"
	"net"
	"strconv"
	"sync"
	"sync/atomic"

	"git.code.oa.com/trpc-go/trpc-go/client"
	"git.code.oa.com/trpc-go/trpc-go/plugin"

	yaml "gopkg.in/yaml.v3"
)

// ServerConfigPath trpc服务配置文件路径，默认是在启动进程当前目录下的trpc_go.yaml，可通过命令行参数 -conf 指定
var ServerConfigPath = "./trpc_go.yaml"

// Config trpc配置实现，分四大块：全局配置global，服务端配置server，客户端配置client，插件配置plugins
type Config struct {
	Global struct {
		Namespace     string `yaml:"namespace"`
		EnvName       string `yaml:"env_name"`
		ContainerName string `yaml:"container_name"`
		LocalIP       string `yaml:"local_ip"`
		EnableSet     string `yaml:"enable_set"`    // Y/N，是否启用Set分组，默认N
		FullSetName   string `yaml:"full_set_name"` // set分组的名字，三段式：[set名].[set地区].[set组名]
	}
	Server struct {
		App      string
		Server   string
		BinPath  string `yaml:"bin_path"`
		DataPath string `yaml:"data_path"`
		ConfPath string `yaml:"conf_path"`
		Admin    struct {
			IP           string `yaml:"ip"` // 要绑定的网卡地址, 如127.0.0.1
			Nic          string
			Port         uint16 `yaml:"port"`          // 要绑定的端口号，如80，默认值9028
			ReadTimeout  int    `yaml:"read_timeout"`  // ms. 请求被接受到请求信息被完全读取的超时时间设置，防止慢客户端
			WriteTimeout int    `yaml:"write_timeout"` // ms. 处理的超时时间
			EnableTLS    bool   `yaml:"enable_tls"`    // 是否启用tls
		}
		Network  string           // 针对所有service的network 默认tcp
		Protocol string           // 针对所有service的protocol 默认trpc
		Filter   []string         // 针对所有service的拦截器
		Service  []*ServiceConfig // 单个service服务的配置
	}
	Client  ClientConfig
	Plugins plugin.Config
}

// ServiceConfig 每个service的配置项，一个服务进程可以支持多个service
type ServiceConfig struct {
	DisableRequestTimeout bool     `yaml:"disable_request_timeout"` // 禁用继承上游的超时时间
	IP                    string   `yaml:"ip"`                      // 监听地址 ip
	Name                  string   // 配置文件定义的用于名字服务的service name:trpc.app.server.service
	Nic                   string   // 监听网卡, 默认情况下由运维分配ip port，此处留空即可，没有分配的情况下 可以支持配置监听网卡
	Port                  uint16   // 监听端口 port
	Address               string   // 监听地址 兼容非ipport模式，有配置address则忽略ipport，没有配置则使用ipport
	Network               string   // 监听网络类型 tcp udp
	Protocol              string   // 业务协议trpc
	Timeout               int      // handler最长处理时间 1s
	Idletime              int      // 长链接空闲时间 5m
	Registry              string   // 使用哪个注册中心 polaris
	Filter                []string // service拦截器
	TLSKey                string   `yaml:"tls_key"`      // server秘钥
	TLSCert               string   `yaml:"tls_cert"`     // server证书
	CACert                string   `yaml:"ca_cert"`      // ca证书，用于校验client证书，以更严格识别客户端的身份，限制客户端的访问
	ServerAsync           bool     `yaml:"server_async"` //启用服务器异步处理
}

// ClientConfig 后端服务配置
type ClientConfig struct {
	Network        string   // 针对所有后端的network 默认tcp
	Protocol       string   // 针对所有后端的protocol 默认trpc
	Filter         []string // 针对所有后端的拦截器
	Namespace      string   // 针对所有后端的namespace
	Timeout        int
	Discovery      string
	Loadbalance    string
	Circuitbreaker string
	Service        []*client.BackendConfig // 单个后端请求的配置
}

// trpc server配置信息，由框架启动后解析yaml文件并赋值
var gm = atomic.Value{}

func init() {
	gm.Store(defaultConfig())
}

func defaultConfig() *Config {
	cfg := &Config{}
	cfg.Global.EnableSet = "N"
	cfg.Server.Network = "tcp"
	cfg.Server.Protocol = "trpc"
	cfg.Client.Network = "tcp"
	cfg.Client.Protocol = "trpc"
	return cfg
}

// GlobalConfig 获取全局配置对象
func GlobalConfig() *Config {
	return gm.Load().(*Config)
}

// SetGlobalConfig 设置全局配置对象
func SetGlobalConfig(cfg *Config) {
	gm.Store(cfg)
}

// LoadGlobalConfig 从配置文件加载配置，并设置到全局结构里面
func LoadGlobalConfig(configPath string) error {
	cfg, err := parseConfigFromFile(configPath)
	if err != nil {
		return err
	}
	RepairConfig(cfg)
	SetGlobalConfig(cfg)
	return nil
}

// LoadConfig 从配置文件加载配置, 并填充好默认值
func LoadConfig(configPath string) (*Config, error) {
	cfg, err := parseConfigFromFile(configPath)
	if err != nil {
		return nil, err
	}
	RepairConfig(cfg)
	return cfg, nil
}

// Setup 根据配置进行初始化工作, 包括注册client配置和执行插件初始化工作
func Setup(cfg *Config) error {
	// 将后端配置注册到框架中，由框架在调用时自动加载配置
	for _, clientCfg := range cfg.Client.Service {
		client.RegisterClientConfig(clientCfg.Callee, clientCfg)
	}

	// * 针对所有后端的通用配置参数
	client.RegisterClientConfig("*", &client.BackendConfig{
		Network:        cfg.Client.Network,
		Protocol:       cfg.Client.Protocol,
		Namespace:      cfg.Client.Namespace,
		Timeout:        cfg.Client.Timeout,
		Filter:         cfg.Client.Filter,
		Discovery:      cfg.Client.Discovery,
		Loadbalance:    cfg.Client.Loadbalance,
		Circuitbreaker: cfg.Client.Circuitbreaker,
	})

	// 装载插件
	if cfg.Plugins != nil {
		err := cfg.Plugins.Setup()
		if err != nil {
			return err
		}
	}

	return nil
}

func parseConfigFromFile(configPath string) (*Config, error) {
	buf, err := ioutil.ReadFile(configPath)
	if err != nil {
		return nil, err
	}

	cfg := defaultConfig()
	err = yaml.Unmarshal(buf, cfg)
	if err != nil {
		return nil, err
	}
	return cfg, nil
}

func repairClientConfig(conf *client.BackendConfig, cfg *ClientConfig) {
	// 默认以proto的service name为key来映射客户端配置，一般proto的service name和后端的service name是相同的，所以默认可不配置
	if conf.Callee == "" {
		conf.Callee = conf.ServiceName
	}
	if conf.ServiceName == "" {
		conf.ServiceName = conf.Callee
	}
	if conf.Namespace == "" {
		conf.Namespace = cfg.Namespace
	}
	if conf.Network == "" {
		conf.Network = cfg.Network
	}
	if conf.Protocol == "" {
		conf.Protocol = cfg.Protocol
	}
	if conf.Target == "" {
		if conf.Discovery == "" {
			conf.Discovery = cfg.Discovery
		}
		if conf.Loadbalance == "" {
			conf.Loadbalance = cfg.Loadbalance
		}
		if conf.Circuitbreaker == "" {
			conf.Circuitbreaker = cfg.Circuitbreaker
		}
	}
	// 优先执行client全局filter, 单个后端配置相同的filter会排重, 不会执行多次
	if len(cfg.Filter) > 0 {
		conf.InsertFilters(cfg.Filter)
	}
}

// RepairConfig 修复配置数据，填充默认值
func RepairConfig(cfg *Config) {
	// nic -> ip
	RepairServiceIPWithNic(cfg)

	// protocol network ip empty
	for _, conf := range cfg.Server.Service {
		if conf.Protocol == "" {
			conf.Protocol = cfg.Server.Protocol
		}
		if conf.Network == "" {
			conf.Network = cfg.Server.Network
		}
		if conf.IP == "" {
			conf.IP = cfg.Global.LocalIP
		}
		if conf.Address == "" {
			conf.Address = net.JoinHostPort(conf.IP, strconv.Itoa(int(conf.Port)))
		}
	}

	if cfg.Client.Namespace == "" {
		cfg.Client.Namespace = cfg.Global.Namespace
	}
	for _, conf := range cfg.Client.Service {
		repairClientConfig(conf, &cfg.Client)
	}
}

// RepairServiceIPWithNic 如果没有设置Service监听的IP，则以监听网卡对应的IP来配置
func RepairServiceIPWithNic(conf *Config) {
	for index, item := range conf.Server.Service {
		if item.IP == "" {
			conf.Server.Service[index].IP = GetIP(item.Nic)
		}
		if conf.Global.LocalIP == "" {
			conf.Global.LocalIP = item.IP
		}
	}

	if conf.Server.Admin.IP == "" {
		conf.Server.Admin.IP = GetIP(conf.Server.Admin.Nic)
	}
}

// NetInterfaceIP 记录本地所有网络接口对应的IP地址
type NetInterfaceIP struct {
	once sync.Once
	ips  map[string]*NicIP
}

// NicIP 记录网卡名对应的IP地址（包括ipv4和ipv6地址）
type NicIP struct {
	nic  string
	ipv4 []string
	ipv6 []string
}

// EnumAllIP 枚举本地网卡对应的ip地址
func (p *NetInterfaceIP) EnumAllIP() map[string]*NicIP {
	p.once.Do(func() {
		p.ips = make(map[string]*NicIP)
		interfaces, err := net.Interfaces()
		if err != nil {
			return
		}
		for _, i := range interfaces {
			p.addInterface(i)
		}
	})
	return p.ips
}

func (p *NetInterfaceIP) addInterface(i net.Interface) {
	addrs, err := i.Addrs()
	if err != nil {
		return
	}
	for _, v := range addrs {
		ipNet, ok := v.(*net.IPNet)
		if !ok {
			continue
		}
		if ipNet.IP.To4() != nil {
			p.addIPv4(i.Name, ipNet.IP.String())
		} else if ipNet.IP.To16() != nil {
			p.addIPv6(i.Name, ipNet.IP.String())
		}
	}
}

// addIPv4 append ipv4 address
func (p *NetInterfaceIP) addIPv4(nic string, ip4 string) {
	ips := p.getNicIP(nic)
	ips.ipv4 = append(ips.ipv4, ip4)
}

// addIPv6 append ipv6 address
func (p *NetInterfaceIP) addIPv6(nic string, ip6 string) {
	ips := p.getNicIP(nic)
	ips.ipv6 = append(ips.ipv6, ip6)
}

// getNicIP 获取网卡名对应的IP地址组
func (p *NetInterfaceIP) getNicIP(nic string) *NicIP {
	if _, ok := p.ips[nic]; !ok {
		p.ips[nic] = &NicIP{nic: nic}
	}
	return p.ips[nic]
}

// GetIPByNic 根据网卡名称返回ip地址
// 优先返回ipv4地址，如果没有ipv4地址，则返回ipv6地址
func (p *NetInterfaceIP) GetIPByNic(nic string) string {
	p.EnumAllIP()
	if len(p.ips) <= 0 {
		return ""
	}
	if _, ok := p.ips[nic]; !ok {
		return ""
	}
	ip := p.ips[nic]
	if len(ip.ipv4) > 0 {
		return ip.ipv4[0]
	}
	if len(ip.ipv6) > 0 {
		return ip.ipv6[0]
	}
	return ""
}

// localIP 记录本地的网卡与IP的对应关系
var localIP = &NetInterfaceIP{}

// GetIP 根据网卡名称返回IP地址
func GetIP(nic string) string {
	ip := localIP.GetIPByNic(nic)
	return ip
}
