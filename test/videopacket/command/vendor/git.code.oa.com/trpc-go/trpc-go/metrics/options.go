package metrics

var options = &Options{}

// Options 上报选项
type Options struct {
	Meta map[string]interface{} // 可以用于适配更多插件场景，如monitor将指标名映射为monitorid
}

// GetOptions 获取参数
func GetOptions() Options {
	return *options
}

// Option 指定插件上报的更多选项，如指定当前上报为模调
type Option func(opts *Options)

// WithMeta 添加metrics相关的一些meta信息，如携带metrics指标与monitorid的映射关系
func WithMeta(meta map[string]interface{}) Option {
	return func(opts *Options) {
		if opts != nil {
			opts.Meta = meta
		}
	}
}
