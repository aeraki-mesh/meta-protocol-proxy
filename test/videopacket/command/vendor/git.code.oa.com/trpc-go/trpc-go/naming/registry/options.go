package registry

// Options 注册节点参数
type Options struct {
	Address string
}

// Option 调用参数工具函数
type Option func(*Options)

// WithAddress 指定server监听地址 ip:port or :port
func WithAddress(s string) Option {
	return func(opts *Options) {
		opts.Address = s
	}
}
