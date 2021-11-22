// Package codec 业务通信打解包协议
package codec

import (
	"sync"
)

// Codec 业务协议打解包接口, 业务协议分成包头head和包体body
// 这里只解析出二进制body，具体业务body结构体通过serializer来处理,
// 一般body都是 pb json jce等，特殊情况可由业务自己注册serializer
type Codec interface {
	// 打包body到二进制buf里面
	// client: Encode(msg, reqbody)(request-buffer, err)
	// server: Encode(msg, rspbody)(response-buffer, err)
	Encode(message Msg, body []byte) (buffer []byte, err error)
	// 从二进制buf里面解出body
	// server: Decode(msg, request-buffer)(reqbody, err)
	// client: Decode(msg, response-buffer)(rspbody, err)
	Decode(message Msg, buffer []byte) (body []byte, err error)
}

var (
	clientCodecs = make(map[string]Codec)
	serverCodecs = make(map[string]Codec)
	lock         sync.RWMutex
)

// Register 通过协议名注册Codec，由第三方具体实现包的init函数调用, 只有client没有server的情况，serverCodec填nil即可
func Register(name string, serverCodec Codec, clientCodec Codec) {
	lock.Lock()
	serverCodecs[name] = serverCodec
	clientCodecs[name] = clientCodec
	lock.Unlock()
}

// GetServer 通过codec name获取server codec
func GetServer(name string) Codec {
	lock.RLock()
	c := serverCodecs[name]
	lock.RUnlock()
	return c
}

// GetClient 通过codec name获取client codec
func GetClient(name string) Codec {
	lock.RLock()
	c := clientCodecs[name]
	lock.RUnlock()
	return c
}
