package metrics

// IGauge is the interface for emitting gauge metrics.
type IGauge interface {
	// Update sets the gauges absolute Value.
	Set(value float64)
}

// gauge 时刻量定义，内部调用注册的插件MetricsSink传递gauge信息到外部系统
type gauge struct {
	name string
}

// Update 更新时刻量
func (g *gauge) Set(v float64) {
	r := NewSingleDimensionMetrics(g.name, v, PolicySET)
	for _, sink := range metricsSinks {
		sink.Report(r)
	}
}
