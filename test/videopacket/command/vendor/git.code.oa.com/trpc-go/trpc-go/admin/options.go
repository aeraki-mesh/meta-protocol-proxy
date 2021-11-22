package admin

import (
	"time"
)

// Option 服务配置选项
type Option func(*adminConfig)

// WithAddr 修改admin绑定的地址, 默认值: ":9028"
// 支持格式:
// 1. :80
// 2. 0.0.0.0:80
// 3. localhost:80
// 4. 127.0.0.0:80
// 5. 10.0.0.2:8001
func WithAddr(addr string) Option {
	return func(config *adminConfig) {
		config.addr = addr
	}
}

// WithTLS 是否使用HTTPS
func WithTLS(isTLS bool) Option {
	return func(config *adminConfig) {
		config.enableTLS = isTLS
	}
}

// WithVersion 由外部传入版本号来设置版本号
func WithVersion(version string) Option {
	return func(config *adminConfig) {
		config.version = version
	}
}

// WithReadTimeout 设置读超时时间
func WithReadTimeout(readTimeout time.Duration) Option {
	return func(config *adminConfig) {
		if readTimeout > 0 {
			config.readTimeout = readTimeout
		}
	}
}

// WithWriteTimeout 设置写超时时间
func WithWriteTimeout(writeTimeout time.Duration) Option {
	return func(config *adminConfig) {
		if writeTimeout > 0 {
			config.writeTimeout = writeTimeout
		}
	}
}

// WithConfigPath 设置框架配置文件路径
func WithConfigPath(configPath string) Option {
	return func(config *adminConfig) {
		config.configPath = configPath
	}
}
