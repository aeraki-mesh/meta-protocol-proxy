package metrics

// NoopSink noop sink
type NoopSink struct {
}

// Name noop name
func (n *NoopSink) Name() string {
	return "noop"
}

// Report  上报一条metrics记录
func (n *NoopSink) Report(rec Record, opts ...Option) error {
	return nil
}
