// Package connpool 连接池
package connpool

import (
	"crypto/tls"
	"crypto/x509"
	"io/ioutil"
	"net"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
)

// GetOptions get conn configuration
type GetOptions struct {
	FramerBuilder codec.FramerBuilder

	CACertFile    string // ca证书
	TLSCertFile   string // client证书
	TLSKeyFile    string // client秘钥
	TLSServerName string // client校验server的服务名, 不填时默认为http的hostname
}

// GetOption Options helper
type GetOption func(*GetOptions)

// WithFramerBuilder 设置 FramerBuilder
func WithFramerBuilder(fb codec.FramerBuilder) GetOption {
	return func(opts *GetOptions) {
		opts.FramerBuilder = fb
	}
}

// WithDialTLS 设置client支持TLS
func WithDialTLS(certFile, keyFile, caFile, serverName string) GetOption {
	return func(opts *GetOptions) {
		opts.TLSCertFile = certFile
		opts.TLSKeyFile = keyFile
		opts.CACertFile = caFile
		opts.TLSServerName = serverName
	}
}

// Pool client connection pool
type Pool interface {
	Get(network string, address string, timeout time.Duration, opt ...GetOption) (net.Conn, error)
}

// DialOptions 请求参数
type DialOptions struct {
	Network       string
	Address       string
	Timeout       time.Duration
	CACertFile    string // ca证书
	TLSCertFile   string // client证书
	TLSKeyFile    string // client秘钥
	TLSServerName string // client校验server的服务名, 不填时默认为http的hostname
}

// Dial 发起请求
func Dial(opts *DialOptions) (net.Conn, error) {
	if len(opts.CACertFile) == 0 {
		return net.DialTimeout(opts.Network, opts.Address, opts.Timeout)
	}

	tlsConf := &tls.Config{}
	if opts.CACertFile == "none" { // 不需要检验服务证书
		tlsConf.InsecureSkipVerify = true
	} else { // 需要校验服务端证书
		if len(opts.TLSServerName) == 0 {
			opts.TLSServerName = opts.Address
		}
		tlsConf.ServerName = opts.TLSServerName
		certPool, err := getCertPool(opts.CACertFile)
		if err != nil {
			return nil, err
		}

		tlsConf.RootCAs = certPool

		if len(opts.TLSCertFile) != 0 { // https服务开启双向认证，需要传送client自身证书给server
			cert, err := tls.LoadX509KeyPair(opts.TLSCertFile, opts.TLSKeyFile)
			if err != nil {
				return nil, errs.NewFrameError(errs.RetClientDecodeFail,
					"client dial load cert file error: "+err.Error())
			}
			tlsConf.Certificates = []tls.Certificate{cert}
		}
	}
	return tls.DialWithDialer(&net.Dialer{Timeout: opts.Timeout}, opts.Network, opts.Address, tlsConf)
}

func getCertPool(caCertFile string) (*x509.CertPool, error) {
	if caCertFile != "root" { // root 表示使用本机安装的root ca证书来验证server，不是root则使用输入ca文件来验证server
		ca, err := ioutil.ReadFile(caCertFile)
		if err != nil {
			return nil, errs.NewFrameError(errs.RetClientDecodeFail,
				"client dial read ca file error: "+err.Error())
		}
		certPool := x509.NewCertPool()
		ok := certPool.AppendCertsFromPEM(ca)
		if !ok {
			return nil, errs.NewFrameError(errs.RetClientDecodeFail,
				"client dial AppendCertsFromPEM fail")
		}

		return certPool, nil
	}

	return nil, nil
}
