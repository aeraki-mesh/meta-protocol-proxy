package servicerouter

// Options 调用参数
type Options struct {
	SourceSetName        string
	DestinationSetName   string
	DisableServiceRouter bool
	Namespace            string
	SourceNamespace      string
	SourceServiceName    string
	SourceEnvName        string
	DestinationEnvName   string
	EnvTransfer          string
	EnvKey               string
	SourceMetadata       map[string]string
	DestinationMetadata  map[string]string
}

// Option 调用参数工具函数
type Option func(*Options)

// WithNamespace 设置 namespace
func WithNamespace(namespace string) Option {

	return func(opts *Options) {
		opts.Namespace = namespace
	}
}

// WithDisableServiceRouter 禁用服务路由
func WithDisableServiceRouter() Option {

	return func(o *Options) {
		o.DisableServiceRouter = true
	}
}

// WithSourceNamespace 指定源服务 namespace
func WithSourceNamespace(namespace string) Option {

	return func(o *Options) {
		o.SourceNamespace = namespace
	}
}

// WithSourceServiceName 指定源服务名
func WithSourceServiceName(serviceName string) Option {

	return func(o *Options) {
		o.SourceServiceName = serviceName
	}
}

// WithSourceEnvName 指定源服务环境
func WithSourceEnvName(envName string) Option {

	return func(o *Options) {
		o.SourceEnvName = envName
	}
}

// WithDestinationEnvName 指定被调服务环境
func WithDestinationEnvName(envName string) Option {

	return func(o *Options) {
		o.DestinationEnvName = envName
	}
}

// WithEnvTransfer 指定透传环境信息
func WithEnvTransfer(envTransfer string) Option {

	return func(o *Options) {
		o.EnvTransfer = envTransfer
	}
}

// WithEnvKey 设置环境 key
func WithEnvKey(key string) Option {

	return func(o *Options) {
		o.EnvKey = key
	}
}

// WithSourceSetName 设置set分组
func WithSourceSetName(sourceSetName string) Option {
	return func(o *Options) {
		o.SourceSetName = sourceSetName
	}
}

// WithDestinationSetName 设置set分组
func WithDestinationSetName(destinationSetName string) Option {
	return func(o *Options) {
		o.DestinationSetName = destinationSetName
	}
}

// WithSourceMetadata 增加主调服务的路由匹配元数据
func WithSourceMetadata(key string, val string) Option {
	return func(o *Options) {
		if o.SourceMetadata == nil {
			o.SourceMetadata = make(map[string]string)
		}
		o.SourceMetadata[key] = val
	}
}

// WithDestinationMetadata 增加被调服务的路由匹配元数据
func WithDestinationMetadata(key string, val string) Option {
	return func(o *Options) {
		if o.DestinationMetadata == nil {
			o.DestinationMetadata = make(map[string]string)
		}
		o.DestinationMetadata[key] = val
	}
}
