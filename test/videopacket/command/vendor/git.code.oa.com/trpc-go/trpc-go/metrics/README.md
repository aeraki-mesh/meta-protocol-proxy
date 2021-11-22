针对框架开发者、插件开发者，Package metrics 支持单维、多维数据的上报，通过以下方法创建单维和多维数据:
- `rec := metrics.NewSingleDimensionMetrics(name, value, policy)`，创建单维数据
- `rec := metrics.NewMultiDimensionMetrics(dimensions, metrics)`，创建多维数据
然后可以通过方法 `metrics.Report(rec)` 进行上报。

针对普通业务开发者，Package metrics 定义了常见粒度的监控指标，如Counter、Gauge、Timer、Histogram，
并在此基础上定义了与具体的外部监控系统对接的接口`type Sink interface`，对接具体的监控如公司Monitor
或者外部开源的Prometheus等，对接不同监控系统时只需要实现`Sink`接口的Plugin就可以。

下面以业务开发者常用的metrics指标为例，说明下使用方法。为了使用方便，提供了两套常用方法：
1. counter
- reqNumCounter := metrics.Counter("req.num")
  reqNumCounter.Incr()
  >这种适用于同一个metrics指标多处使用的情况
- metrics.IncrCounter("req.num", 1)
  >这种适用于只适用于代码中只使用一次的情况

2. gauge
- cpuAvgLoad := metrics.Gauge("cpu.avgload")
  cpuAvgLoad.Set(0.75)
  >这种适用于同一个metrics指标多处使用的情况
- metrics.SetGauge("cpu.avgload", 0.75)
  >这种适用于只适用于代码中只使用一次的情况

3. timer
- timeCostTimer := metrics.Timer("req.proc.timecost")
  timeCostTimer.Record()
  >这种适用于同一个metrics指标多处使用的情况
- timeCostDuration := time.Millisecond * 2000
  metrics.RecordTimer("req.proc.timecost", timeCostDuration)
  >这种适用于只适用于代码中只使用一次的情况
                                                                                                             >
4. histogram
- buckets := metrics.NewDurationBounds(time.Second, time.Second*2, time.Second*5),
  timeCostDist := metrics.Histogram("timecost.distribution", buckets)
  timeCostDist.AddSample(float64(time.Second*3))
  >这种适用于同一个metrics指标多处使用的情况
- metrics.AddSample("timecost.distribution", buckets, float64(time.Second*3))
  >这种适用于只适用于代码中只使用一次的情况