package rollwriter

// Options 客户端调用参数
type Options struct {
	// MaxSize 日志文件最大大小(字节)
	MaxSize int64

	// MaxBackups 保留的日志最大文件数
	MaxBackups int

	// MaxAge 日志最大保留时间(天)
	MaxAge int

	// 日志文件是否压缩
	Compress bool

	// TimeFormat 按时间分割文件的时间格式
	TimeFormat string
}

// Option 调用参数工具函数
type Option func(*Options)

// WithMaxSize 设置日志文件最大大小(MB)
func WithMaxSize(n int) Option {
	return func(o *Options) {
		o.MaxSize = int64(n) * 1024 * 1024
	}
}

// WithMaxAge 设置日志最大保留时间（天）
func WithMaxAge(n int) Option {
	return func(o *Options) {
		o.MaxAge = n
	}
}

// WithMaxBackups 设置日志保留的最大文件数
func WithMaxBackups(n int) Option {
	return func(o *Options) {
		o.MaxBackups = n
	}
}

// WithCompress 设置是否压错
func WithCompress(b bool) Option {
	return func(o *Options) {
		o.Compress = b
	}
}

// WithRotationTime 设置按时间滚动的时间格式(如：%Y%m%d)
func WithRotationTime(s string) Option {
	return func(o *Options) {
		o.TimeFormat = s
	}
}
