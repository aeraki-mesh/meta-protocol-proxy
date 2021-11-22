// Package metrics 支持单维、多维数据上报
package metrics

// Policy 指标值聚合策略
type Policy int

// 策略名
const (
	PolicyNONE      = 0 // 未指定
	PolicySET       = 1 // 瞬时值
	PolicySUM       = 2 // 求和
	PolicyAVG       = 3 // 求平均值
	PolicyMAX       = 4 // 求最大值
	PolicyMIN       = 5 // 求最小值
	PolicyMID       = 6 // 求中位数
	PolicyTimer     = 7 // 计时器
	PolicyHistogram = 8 // 直方图统计
)

// Sink 监控系统对接接口
//
// Report, 上报监控信息
type Sink interface {
	Name() string
	Report(rec Record, opts ...Option) error
}

// Record 上报记录
//
// 一条记录中涉及到的一些术语包括：
// - 维度名
//   是数据的属性，一般被用来过滤数据，如相册业务模块包括地区、机房等维度;
// - 维度值
//   是对维度的细化，如相册业务模块地区包括深圳、上海等，地区是维度，深圳、上海为维度值;
// - 指标
//   是一种度量字段，用来做聚合或计算，如相册业务模块在深圳电信请求数，请求数是一个指标;
type Record struct {
	Name       string       // 监控项名字
	dimensions []*Dimension // 维度名，如地区、主机、磁盘编号; 维度值，如地区=上海
	metrics    []*Metrics
}

// Dimension 多维数据维度信息
type Dimension struct {
	Name  string
	Value string
}

// GetDimensions 返回包含的维度信息
func (r *Record) GetDimensions() []*Dimension {
	if r == nil {
		return nil
	}
	return r.dimensions
}

// GetMetrics 返回包含的指标信息
func (r *Record) GetMetrics() []*Metrics {
	if r == nil {
		return nil
	}
	return r.metrics
}

// NewSingleDimensionMetrics 创建一个单维上报记录
func NewSingleDimensionMetrics(name string, value float64, policy Policy) Record {
	r := Record{
		dimensions: nil,
		metrics: []*Metrics{
			{name: name, value: value, policy: policy},
		},
	}
	return r
}

// ReportSingleDimensionMetrics 创建一个单维上报记录，并上报
func ReportSingleDimensionMetrics(name string, value float64, policy Policy) error {
	r := Record{
		dimensions: nil,
		metrics: []*Metrics{
			{name: name, value: value, policy: policy},
		},
	}
	return Report(r)
}

// NewMultiDimensionMetrics 创建一个多维上报记录
func NewMultiDimensionMetrics(dimensions []*Dimension, metrics []*Metrics) Record {
	r := Record{
		dimensions: dimensions,
		metrics:    metrics,
	}
	return r
}

// ReportMultiDimensionMetrics 创建一个多维上报记录，并上报
func ReportMultiDimensionMetrics(dimensions []*Dimension, metrics []*Metrics) error {
	r := Record{
		dimensions: dimensions,
		metrics:    metrics,
	}
	return Report(r)
}

// Metrics 指标
type Metrics struct {
	name   string  // 指标名
	value  float64 // 指标值
	policy Policy  // 聚合策略
}

// NewMetrics 创建一个指标
func NewMetrics(name string, value float64, policy Policy) *Metrics {
	return &Metrics{name, value, policy}
}

// Name 返回指标名
func (m *Metrics) Name() string {
	if m == nil {
		return ""
	}
	return m.name
}

// Value 返回指标值
func (m *Metrics) Value() float64 {
	if m == nil {
		return 0
	}
	return m.value
}

// Policy 返回指标聚合策略
func (m *Metrics) Policy() Policy {
	if m == nil {
		return PolicyNONE
	}
	return m.policy
}
