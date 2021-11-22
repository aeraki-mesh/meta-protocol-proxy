// Package metrics 定义了常见粒度的监控指标，如Counter、IGauge、ITimer、IHistogram，
// 并在此基础上定义了与具体的外部监控系统对接的接口MetricsSink，对接具体的监控如公司
// Monitor或者外部开源的Prometheus等，只需是吸纳对应的MetricsSink接口即可.
//
// 为了使用方便，提供了两套常用方法：
// 1. counter
// - reqNumCounter := metrics.Counter("req.num")
//   reqNumCounter.Incr()
// - metrics.IncrCounter("req.num", 1)
//
// 2. gauge
// - cpuAvgLoad := metrics.Gauge("cpu.avgload")
//   cpuAvgLoad.Set(0.75)
// - metrics.SetGauge("cpu.avgload", 0.75)
//
// 3. timer
// - timeCostTimer := metrics.Timer("req.proc.timecost")
//   timeCostTimer.Record()
// - timeCostDuration := time.Millisecond * 2000
//   metrics.RecordTimer("req.proc.timecost", timeCostDuration)
//
// 4. histogram
// - buckets := metrics.NewDurationBounds(time.Second, time.Second*2, time.Second*5),
//   timeCostDist := metrics.Histogram("timecost.distribution", buckets)
//   timeCostDist.AddSample(float64(time.Second*3))
// - metrics.AddSample("timecost.distribution", buckets, float64(time.Second*3))
package metrics

import (
	"fmt"
	"sync"
	"time"
)

var (
	// allow emit same metrics information to multi external system at same time
	metricsSinks    = map[string]Sink{}
	muxMetricsSinks = sync.RWMutex{}

	counters   = map[string]ICounter{}
	gauges     = map[string]IGauge{}
	timers     = map[string]ITimer{}
	histograms = map[string]IHistogram{}

	lockCounters   = sync.RWMutex{}
	lockGauges     = sync.RWMutex{}
	lockTimers     = sync.RWMutex{}
	lockHistograms = sync.RWMutex{}

	meta = map[string]interface{}{}
)

// RegisterMetricsSink register one Sink
func RegisterMetricsSink(sink Sink) {
	muxMetricsSinks.Lock()
	metricsSinks[sink.Name()] = sink
	muxMetricsSinks.Unlock()
}

// Counter create a counter named `Name`
func Counter(name string) ICounter {

	lockCounters.RLock()
	c, ok := counters[name]
	lockCounters.RUnlock()

	if ok && c != nil {
		return c
	}

	lockCounters.Lock()

	c, ok = counters[name]
	if ok && c != nil {
		lockCounters.Unlock()
		return c
	}

	c = &counter{name: name}
	counters[name] = c

	lockCounters.Unlock()

	return c
}

// Gauge create a gauge named `Name`
func Gauge(name string) IGauge {

	lockGauges.RLock()
	c, ok := gauges[name]
	lockGauges.RUnlock()

	if ok && c != nil {
		return c
	}

	lockGauges.Lock()

	c, ok = gauges[name]
	if ok && c != nil {
		lockGauges.Unlock()
		return c
	}

	c = &gauge{name: name}
	gauges[name] = c

	lockGauges.Unlock()

	return c
}

// Timer create a timer named `Name`
func Timer(name string) ITimer {

	lockTimers.RLock()
	t, ok := timers[name]
	lockTimers.RUnlock()

	if ok && t != nil {
		return t
	}

	lockTimers.Lock()

	t, ok = timers[name]
	if ok && t != nil {
		lockTimers.Unlock()
		return t
	}

	t = &timer{name: name, start: time.Now()}
	timers[name] = t

	lockTimers.Unlock()

	return t
}

// Histogram create a histogram named `Name` with `Buckets`
func Histogram(name string, buckets BucketBounds) IHistogram {

	h, ok := GetHistogram(name)
	if ok && h != nil {
		return h
	}

	lockHistograms.Lock()
	defer lockHistograms.Unlock()

	h, ok = histograms[name]
	if ok && h != nil {
		return h
	}

	h = newHistogram(name, buckets)
	histograms[name] = h

	return h
}

// GetHistogram 获得key对应的直方图
func GetHistogram(key string) (v IHistogram, ok bool) {
	lockHistograms.RLock()
	h, ok := histograms[key]
	lockHistograms.RUnlock()
	if !ok {
		return nil, false
	}

	hist, ok := h.(*histogram)
	if !ok {
		return nil, false
	}

	return hist, true
}

// IncrCounter increment counter `key` by `value`, Counters should accumulate values
func IncrCounter(key string, value float64) {
	Counter(key).IncrBy(value)
}

// SetGauge set gauge `key` with `value`, a IGauge should retain the last value it is set to
func SetGauge(key string, value float64) {
	Gauge(key).Set(value)
}

// RecordTimer record timer named `key` with `duration`
func RecordTimer(key string, duration time.Duration) {
	Timer(key).RecordDuration(duration)
}

// AddSample add one sample `key` with `value`, Samples are added to histogram for timing or value distribution case
func AddSample(key string, buckets BucketBounds, value float64) {

	h := Histogram(key, buckets)
	h.AddSample(value)
}

// Report 上报多维信息
func Report(rec Record) (err error) {

	var errs []error

	for _, sink := range metricsSinks {
		err = sink.Report(rec)
		if err != nil {
			errs = append(errs, fmt.Errorf("sink-%s error: %v", sink.Name(), err))
		}
	}

	if len(errs) == 0 {
		return nil
	}
	return fmt.Errorf("metrics sink error: %v", errs)
}
