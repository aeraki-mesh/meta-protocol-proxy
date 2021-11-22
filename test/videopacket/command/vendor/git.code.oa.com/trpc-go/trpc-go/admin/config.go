package admin

import (
	"time"
)

const (
	defaultListenAddr   = "127.0.0.1:9028" // 默认监听端口
	defaultUseTLS       = false            // 默认是否使用https
	defaultReadTimeout  = time.Second * 3
	defaultWriteTimeout = time.Second * 60
)

func loadDefaultConfig() adminConfig {
	return adminConfig{
		addr:         defaultListenAddr,
		enableTLS:    defaultUseTLS,
		readTimeout:  defaultReadTimeout,
		writeTimeout: defaultWriteTimeout,
	}
}

type adminConfig struct {
	addr         string
	enableTLS    bool
	readTimeout  time.Duration
	writeTimeout time.Duration
	version      string
	configPath   string
}

func (a *adminConfig) getAddr() string {
	return a.addr
}
