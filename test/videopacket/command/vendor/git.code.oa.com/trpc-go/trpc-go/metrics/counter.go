package metrics

// ICounter is the interface for emitting counter type metrics.
type ICounter interface {
	// Incr increments the counter by one.
	Incr()

	// IncrBy increments the counter by a delta.
	IncrBy(delta float64)
}

// counter 计数器定义, 内部调用注册的插件MetricsSink传递counter信息到外部系统
type counter struct {
	name string
}

// Inc 计数器值+1
func (c *counter) Incr() {
	c.IncrBy(1)
}

// Inc 计数器值增加v
func (c *counter) IncrBy(v float64) {
	rec :=  NewSingleDimensionMetrics(c.name, v, PolicySUM)
	for _, sink := range metricsSinks {
		sink.Report(rec)
	}
}
