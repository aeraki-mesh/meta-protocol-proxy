package metrics

import (
	"encoding/json"
	"fmt"
	"sort"
	"sync"
	"time"
)

// NewConsoleSink new console sink
func NewConsoleSink() Sink {
	return &ConsoleSink{
		counters:   make(map[string]float64),
		gauges:     make(map[string]float64),
		timers:     make(map[string]timer),
		histograms: make(map[string]histogram),
		cm:         sync.RWMutex{},
		gm:         sync.RWMutex{},
		tm:         sync.RWMutex{},
		hm:         sync.RWMutex{},
	}
}

// ConsoleSink console sink
type ConsoleSink struct {
	counters   map[string]float64
	gauges     map[string]float64
	timers     map[string]timer
	histograms map[string]histogram

	cm sync.RWMutex
	gm sync.RWMutex
	tm sync.RWMutex
	hm sync.RWMutex
}

// Name console sink name
func (c *ConsoleSink) Name() string {
	return "console"
}

// Report 上报一条metrics记录
func (c *ConsoleSink) Report(rec Record, opts ...Option) error {

	if len(rec.dimensions) <= 0 {
		return c.reportSingleDimensionMetrics(rec, opts...)
	}

	return c.reportMultiDimensionMetrics(rec, opts...)
}

func (c *ConsoleSink) reportSingleDimensionMetrics(rec Record, opts ...Option) error {
	// 常见的累积量、时刻量，绝大多数监控系统都是支持的
	for _, m := range rec.metrics {
		switch m.policy {
		case PolicySUM:
			c.incrCounter(m.name, m.value)
		case PolicySET:
			c.setGauge(m.name, m.value)
		case PolicyTimer:
			c.recordTimer(m.name, time.Duration(m.value))
		case PolicyHistogram:
			c.addSample(m.name, m.value)
		default:
			// not supported policies
		}
	}
	return nil
}

func (c *ConsoleSink) reportMultiDimensionMetrics(rec Record, opts ...Option) error {

	options := Options{}
	for _, o := range opts {
		o(&options)
	}

	buf, err := json.Marshal(rec)
	if err != nil {
		return err
	}

	// 普通的多维监控上报
	fmt.Printf("metrics multi-dimension = %s", string(buf))

	return nil
}

// incrCounter incr counter
func (c *ConsoleSink) incrCounter(key string, value float64) {
	if c.counters == nil {
		return
	}
	c.cm.Lock()
	c.counters[key]+=value
	c.cm.Unlock()
	fmt.Printf("metrics counter[key] = %s val = %v", key, value)
}

// setGauge set gauge
func (c *ConsoleSink) setGauge(key string, value float64) {
	if c.gauges == nil {
		return
	}
	c.cm.Lock()
	c.gauges[key] = value
	c.cm.Unlock()
	fmt.Printf("metrics gauge[key] = %s val = %v\n", key, value)
}

// recordTimer record timer
func (c *ConsoleSink) recordTimer(key string, duration time.Duration) {
	fmt.Printf("metrics timer[key] = %s val = %v\n", key, duration)
}

// addSample add sample
func (c *ConsoleSink) addSample(key string, value float64) {

	lockHistograms.RLock()
	h := histograms[key]
	lockHistograms.RUnlock()

	v, ok := h.(*histogram)
	if !ok {
		return
	}
	//c.hm.Lock()
	hist := *v

	lockHistograms.Lock()
	idx := sort.SearchFloat64s(hist.LookupByValue, value)
	upperBound := hist.Buckets[idx].ValueUpperBound
	hist.Buckets[idx].samples += value
	lockHistograms.Unlock()

	fmt.Printf("metrics histogram[%s.%v] = %v", hist.Name, upperBound, hist.Buckets[idx].samples)
}
