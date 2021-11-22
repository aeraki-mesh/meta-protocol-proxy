package discovery

// Options 调用参数
type Options struct {
	Namespace string
}

// Option 调用参数工具函数
type Option func(*Options)

// WithNamespace 设置 namespace
func WithNamespace(namespace string) Option {
	return func(opts *Options) {
		opts.Namespace = namespace
	}
}
