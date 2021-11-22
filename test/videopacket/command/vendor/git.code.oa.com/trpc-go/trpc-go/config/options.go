package config

// WithCodec 使用指定名字的Codec
func WithCodec(name string) LoadOption {
	return func(c *TrpcConfig) {
		c.decoder = GetCodec(name)
	}
}

// WithProvider 使用指定名字的Provider
func WithProvider(name string) LoadOption {
	return func(c *TrpcConfig) {
		c.p = GetProvider(name)
	}
}

type options struct{}

// Option 配置中心SDK选项
type Option func(*options)
