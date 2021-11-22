package connpool

import (
	"time"
)

// Options indicate pool configuration
type Options struct {
	MinIdle         int           // 最低闲置连接数量，为下次io做好准备
	MaxIdle         int           // 最大闲置连接数量，0 代表不做闲置
	MaxActive       int           // 最大活跃连接数量，0 代表不做限制
	Wait            bool          // 活跃连接达到最大数量时，是否等待
	IdleTimeout     time.Duration // 空闲连接超时时间
	MaxConnLifetime time.Duration // 连接的最大生命周期
	DialTimeout     time.Duration // 建立连接超时时间
}

// Option Options helper
type Option func(*Options)

// WithMinIdle 指定最小空闲连接数
func WithMinIdle(n int) Option {
	return func(o *Options) {
		o.MinIdle = n
	}
}

// WithMaxIdle 指定最大空闲连接数
func WithMaxIdle(m int) Option {
	return func(o *Options) {
		o.MaxIdle = m
	}
}

// WithMaxActive 指定最大活动连接数
func WithMaxActive(s int) Option {
	return func(o *Options) {
		o.MaxActive = s
	}
}

// WithWait 连接数达到限制时是否等待
func WithWait(w bool) Option {
	return func(o *Options) {
		o.Wait = w
	}
}

// WithIdleTimeout 指定空闲连接时间，超过这个时间将可能被关闭
func WithIdleTimeout(t time.Duration) Option {
	return func(o *Options) {
		o.IdleTimeout = t
	}
}

// WithMaxConnLifetime 指定连接最大生存时间，超过这个时间将可能被关闭
func WithMaxConnLifetime(t time.Duration) Option {
	return func(o *Options) {
		o.MaxConnLifetime = t
	}
}

// WithDialTimeout 指定连接池建立连接的默认超时时间
func WithDialTimeout(t time.Duration) Option {
	return func(o *Options) {
		o.DialTimeout = t
	}
}
