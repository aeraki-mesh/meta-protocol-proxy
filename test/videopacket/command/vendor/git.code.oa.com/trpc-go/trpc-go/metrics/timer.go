package metrics

import (
	"time"
)

// ITimer is the interface for emitting timer metrics.
type ITimer interface {
	// Record a specific duration directly.
	Record() time.Duration
	RecordDuration(duration time.Duration)
}

// timer 计时器定义，内部调用注册的插件MetricsSink传递timer信息到外部系统
type timer struct {
	name  string
	start time.Time
}

// Record 记录定时器耗时
func (t *timer) Record() time.Duration {
	duration := time.Since(t.start)
	r := NewSingleDimensionMetrics(t.name, float64(duration), PolicyTimer)
	for _, sink := range metricsSinks {
		sink.Report(r)
	}
	return duration
}

// RecordDuration 记录定时器耗时为duration, 忽略t.start
func (t *timer) RecordDuration(duration time.Duration) {
	r := NewSingleDimensionMetrics(t.name, float64(duration), PolicyTimer)
	for _, sink := range metricsSinks {
		sink.Report(r)
	}
}
