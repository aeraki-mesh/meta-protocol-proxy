package metrics

import (
	"math"
	"sort"
	"sync"
	"time"
)

// IHistogram is the interface for emitting histogram metrics
type IHistogram interface {
	// addSample records a sample into histogram
	AddSample(value float64)
}

// histogram 直方图定义，直方图将加入的样本点按照预先规划好的buckets进行分组,
// 内部调用注册的插件MetricsSink传递gauge信息到外部系统
type histogram struct {
	Name          string
	Meta          map[string]interface{}
	Spec          BucketBounds
	Buckets       []*bucket
	LookupByValue []float64
}

// newHistogram 创建新的直方图，直方图名称为`Name`，直方图中buckets的分界范围定义`Buckets`
func newHistogram(name string, buckets BucketBounds) *histogram {

	h := &histogram{
		Name:          name,
		Spec:          buckets,
		Buckets:       make([]*bucket, 0),
		LookupByValue: make([]float64, 0),
	}

	ranges := ranges(buckets)

	for _, r := range ranges {
		b := bucket{
			h: h,
			// fixme +Inf, -Inf
			samples:         0,
			ValueLowerBound: r.lowerBoundValue,
			ValueUpperBound: r.upperBoundValue,
		}
		h.addBucket(&b)
	}

	return h
}

// addSample 加入新的样本点
func (h *histogram) AddSample(value float64) {
	idx := sort.SearchFloat64s(h.LookupByValue, value)
	h.Buckets[idx].lock.Lock()
	h.Buckets[idx].samples += value
	h.Buckets[idx].lock.Unlock()

	r := NewSingleDimensionMetrics(h.Name, value, PolicyHistogram)
	for _, sink := range metricsSinks {
		sink.Report(r)
	}
}

// addBucket 直方图中添加新的bucket
func (h *histogram) addBucket(b *bucket) {
	h.Buckets = append(h.Buckets, b)
	h.LookupByValue = append(h.LookupByValue, b.ValueUpperBound)
}

// BucketBounds allow developers customize Buckets of histogram
type BucketBounds []float64

// NewValueBounds new value bounds
func NewValueBounds(bounds ...float64) BucketBounds {
	return BucketBounds(bounds)
}

// NewDurationBounds new duration bounds
func NewDurationBounds(durations ...time.Duration) BucketBounds {
	bounds := []float64{}
	for _, v := range durations {
		bounds = append(bounds, float64(v))
	}
	return BucketBounds(bounds)
}

// Implements sort.Interface
func (v BucketBounds) Len() int {
	return len(v)
}

// Implements sort.Interface
func (v BucketBounds) Swap(i, j int) {
	v[i], v[j] = v[j], v[i]
}

// Implements sort.Interface
func (v BucketBounds) Less(i, j int) bool {
	return v[i] < v[j]
}

func (v BucketBounds) sorted() []float64 {
	valuesCopy := clone(v)
	sort.Sort(BucketBounds(valuesCopy))
	return valuesCopy
}

// bucket several Buckets assemble the histogram, every bucket contains an counter
type bucket struct {
	h *histogram
	// fixme benchmark, using atomic to optimize
	lock            sync.Mutex
	samples         float64
	ValueLowerBound float64
	ValueUpperBound float64
}

// ranges creates a set of bucket pairs from a set of Buckets describing the lower
// and upper ranges for each derived bucket.
func ranges(buckets BucketBounds) []bucketRange {

	if len(buckets) < 1 {
		s := bucketRange{lowerBoundValue: -math.MaxFloat64, upperBoundValue: math.MaxFloat64}
		return []bucketRange{s}
	}

	// if Buckets range is [A,B), don't forget (~,A) and [B,~)
	ranges := make([]bucketRange, 0, buckets.Len()+2)

	sortedBounds := buckets.sorted()

	lowerBoundValue := -math.MaxFloat64
	for i := 0; i < buckets.Len(); i++ {
		ranges = append(ranges, bucketRange{lowerBoundValue: lowerBoundValue, upperBoundValue: sortedBounds[i]})
		lowerBoundValue = sortedBounds[i]
	}

	ranges = append(ranges, bucketRange{
		lowerBoundValue: sortedBounds[len(sortedBounds)-1],
		upperBoundValue: math.MaxFloat64,
	})

	return ranges
}

// clone 深拷贝一个slice
func clone(values []float64) []float64 {
	valuesCopy := make([]float64, len(values))
	for i := range values {
		valuesCopy[i] = values[i]
	}
	return valuesCopy
}

type bucketRange struct {
	lowerBoundValue float64
	upperBoundValue float64
}
