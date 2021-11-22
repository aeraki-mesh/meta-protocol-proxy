package codec

import (
	"context"
	"net"
	"strings"
	"sync"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/errs"
)

// ContextKey trpc context key type，context value取值是通过接口判断的，接口会同时判断值和类型，定义新type可以防止重复字符串导致冲突覆盖问题
type ContextKey string

// MetaData 请求message透传字段信息
type MetaData map[string][]byte

var msgPool = sync.Pool{
	New: func() interface{} {
		return &msg{}
	},
}

// Clone 复制一个新的metadata
func (m MetaData) Clone() MetaData {

	md := MetaData{}
	for k, v := range m {
		md[k] = v
	}
	return md
}

// trpc context key data trpc基本数据
const (
	ContextKeyMessage    = ContextKey("TRPC_MESSAGE")
	ServiceSectionLength = 4 // 点分4段式的trpc服务名称trpc.app.server.service
)

// Msg 多协议通用的消息数据 业务协议打解包的时候需要设置message内部信息
type Msg interface {
	Context() context.Context

	WithRemoteAddr(addr net.Addr) // server transport 设置上游地址， client 设置下游地址
	WithLocalAddr(addr net.Addr)  // server transport 设置本地地址
	RemoteAddr() net.Addr
	LocalAddr() net.Addr
	WithNamespace(string) // server 的 namespace
	Namespace() string

	WithEnvName(string) // server 的环境
	EnvName() string

	WithSetName(string) //server所在的set
	SetName() string

	WithEnvTransfer(string) // 服务透传的环境信息
	EnvTransfer() string

	WithRequestTimeout(time.Duration) // server codec 设置上游超时，client设置下游超时
	RequestTimeout() time.Duration

	WithSerializationType(int)
	SerializationType() int

	WithCompressType(int)
	CompressType() int

	WithServerRPCName(string) // server codec 设置当前server handler调用方法名
	WithClientRPCName(string) // client stub 设置下游调用方法名

	ServerRPCName() string // 当前server handler调用方法名：/trpc.app.server.service/method
	ClientRPCName() string // 调用下游的接口方法名

	WithCallerServiceName(string) // 调用方服务名
	WithCalleeServiceName(string) // 被调方服务名

	WithCallerApp(string) // 主调app server角度是上游的app，client角度是自身的app
	WithCallerServer(string)
	WithCallerService(string)
	WithCallerMethod(string)

	WithCalleeApp(string) // 被调app server角度是自身app，client角度是下游的app
	WithCalleeServer(string)
	WithCalleeService(string)
	WithCalleeMethod(string)

	CallerServiceName() string // 主调服务名：trpc.app.server.service server角度是上游服务名，client角度是自身的服务名
	CallerApp() string         // 主调app server角度是上游的app，client角度是自身的app
	CallerServer() string
	CallerService() string
	CallerMethod() string

	CalleeServiceName() string // 被调服务名 server角度是自身服务名，client角度是下游的服务名
	CalleeApp() string         // 被调app server角度是自身app，client角度是下游的app
	CalleeServer() string
	CalleeService() string
	CalleeMethod() string

	CalleeContainerName() string // 被调服务容器名
	WithCalleeContainerName(string)

	WithServerMetaData(MetaData)
	ServerMetaData() MetaData

	WithFrameHead(interface{})
	FrameHead() interface{}

	WithServerReqHead(interface{})
	ServerReqHead() interface{}

	WithServerRspHead(interface{})
	ServerRspHead() interface{}

	WithDyeing(bool)
	Dyeing() bool

	WithDyeingKey(string)
	DyeingKey() string

	WithServerRspErr(error)
	ServerRspErr() *errs.Error

	WithClientMetaData(MetaData)
	ClientMetaData() MetaData

	WithClientReqHead(interface{})
	ClientReqHead() interface{}

	WithClientRspErr(error)
	ClientRspErr() error

	WithClientRspHead(interface{})
	ClientRspHead() interface{}

	WithLogger(l interface{})
	Logger() interface{}
}

type msg struct {
	context             context.Context
	frameHead           interface{}
	requestTimeout      time.Duration
	serializationType   int
	compressType        int
	dyeing              bool
	dyeingKey           string
	serverRPCName       string
	clientRPCName       string
	serverMetaData      MetaData
	clientMetaData      MetaData
	callerServiceName   string
	calleeServiceName   string
	calleeContainerName string
	serverRspErr        error
	clientRspErr        error
	serverReqHead       interface{}
	serverRspHead       interface{}
	clientReqHead       interface{}
	clientRspHead       interface{}
	localAddr           net.Addr
	remoteAddr          net.Addr
	logger              interface{}
	callerApp           string
	callerServer        string
	callerService       string
	callerMethod        string
	calleeApp           string
	calleeServer        string
	calleeService       string
	calleeMethod        string
	namespace           string
	setName             string
	envName             string
	envTransfer         string
}

// resetDefault 将msg的所有成员变量变成默认值
func (m *msg) resetDefault() {
	m.context = nil
	m.frameHead = nil
	m.requestTimeout = 0
	m.serializationType = 0
	m.compressType = 0
	m.dyeing = false
	m.dyeingKey = ""
	m.serverRPCName = ""
	m.clientRPCName = ""
	m.serverMetaData = nil
	m.clientMetaData = nil
	m.callerServiceName = ""
	m.calleeServiceName = ""
	m.calleeContainerName = ""
	m.serverRspErr = nil
	m.clientRspErr = nil
	m.serverReqHead = nil
	m.serverRspHead = nil
	m.clientReqHead = nil
	m.clientRspHead = nil
	m.localAddr = nil
	m.remoteAddr = nil
	m.logger = nil
	m.callerApp = ""
	m.callerServer = ""
	m.callerService = ""
	m.callerMethod = ""
	m.calleeApp = ""
	m.calleeServer = ""
	m.calleeService = ""
	m.calleeMethod = ""
	m.namespace = ""
	m.setName = ""
	m.envName = ""
	m.envTransfer = ""

}

// Context 新建msg时，保存老的ctx
func (m *msg) Context() context.Context {
	return m.context
}

// WithNamespace 设置 server 的 namespace
func (m *msg) WithNamespace(namespace string) {
	m.namespace = namespace
}

// Namespace 返回 namespace
func (m *msg) Namespace() string {
	return m.namespace
}

// WithEnvName 设置环境
func (m *msg) WithEnvName(envName string) {
	m.envName = envName
}

// WithSetName 设置set分组
func (m *msg) WithSetName(setName string) {
	m.setName = setName
}

// SetName 返回set分组
func (m *msg) SetName() string {
	return m.setName
}

// EnvName 返回环境
func (m *msg) EnvName() string {
	return m.envName
}

// WithEnvTransfer 设置透传环境信息
func (m *msg) WithEnvTransfer(envTransfer string) {
	m.envTransfer = envTransfer
}

// EnvTransfer 返回透传环境信息
func (m *msg) EnvTransfer() string {
	return m.envTransfer
}

// WithRemoteAddr 设置 remoteAddr
func (m *msg) WithRemoteAddr(addr net.Addr) {
	m.remoteAddr = addr
}

// WithLocalAddr 设置 localAddr
func (m *msg) WithLocalAddr(addr net.Addr) {
	m.localAddr = addr
}

// RemoteAddr 获取 remoteAddr
func (m *msg) RemoteAddr() net.Addr {
	return m.remoteAddr
}

// LocalAddr 获取 localAddr
func (m *msg) LocalAddr() net.Addr {
	return m.localAddr
}

// RequestTimeout 上游业务协议里面设置的请求超时时间
func (m *msg) RequestTimeout() time.Duration {
	return m.requestTimeout
}

// WithRequestTimeout 设置请求超时时间
func (m *msg) WithRequestTimeout(t time.Duration) {
	m.requestTimeout = t
}

// FrameHead 返回桢信息
func (m *msg) FrameHead() interface{} {
	return m.frameHead
}

// WithFrameHead 设置桢信息
func (m *msg) WithFrameHead(f interface{}) {
	m.frameHead = f
}

// SerializationType body序列化方式 见 serialization.go里面的常量定义
func (m *msg) SerializationType() int {
	return m.serializationType
}

// WithSerializationType 设置body序列化方式
func (m *msg) WithSerializationType(t int) {
	m.serializationType = t
}

// CompressType body解压缩方式 见 compress.go里面的常量定义
func (m *msg) CompressType() int {
	return m.compressType
}

// WithCompressType 设置body解压缩方式
func (m *msg) WithCompressType(t int) {
	m.compressType = t
}

// ServerRPCName 服务端接收到请求的协议的rpc name
func (m *msg) ServerRPCName() string {
	return m.serverRPCName
}

// WithServerRPCName 设置服务端rpc name
func (m *msg) WithServerRPCName(s string) {
	if m.serverRPCName == s {
		return
	}
	m.serverRPCName = s
	method := strings.Split(s, "/") // /trpc.app.server.service/method
	if len(method) == 3 {
		m.WithCalleeMethod(method[2])
	}
}

// ClientRPCName 后端调用设置的rpc name
func (m *msg) ClientRPCName() string {
	return m.clientRPCName
}

// WithClientRPCName 设置后端调用rpc name，由client stub调用
func (m *msg) WithClientRPCName(s string) {
	if m.clientRPCName == s {
		return
	}
	m.clientRPCName = s
	method := strings.Split(s, "/") // /trpc.app.server.service/method
	if len(method) == 3 {
		m.WithCalleeMethod(method[2])
	}
}

// ServerMetaData 服务端收到请求的协议透传字段
func (m *msg) ServerMetaData() MetaData {
	return m.serverMetaData
}

// WithServerMetaData 设置服务端收到请求的协议透传字段
func (m *msg) WithServerMetaData(d MetaData) {
	if d == nil {
		d = MetaData{}
	}
	m.serverMetaData = d
}

// ClientMetaData 调用下游时的客户端协议透传字段
func (m *msg) ClientMetaData() MetaData {
	return m.clientMetaData
}

// WithClientMetaData 设置调用下游时的客户端协议透传字段
func (m *msg) WithClientMetaData(d MetaData) {
	if d == nil {
		d = MetaData{}
	}
	m.clientMetaData = d
}

// CalleeServiceName 被调方的服务名
func (m *msg) CalleeServiceName() string {
	return m.calleeServiceName
}

// WithCalleeServiceName 设置被调方的服务名
func (m *msg) WithCalleeServiceName(s string) {
	if m.calleeServiceName == s {
		return
	}
	m.calleeServiceName = s
	callee := strings.Split(s, ".")
	if len(callee) < ServiceSectionLength {
		return
	}
	m.WithCalleeApp(callee[1])
	m.WithCalleeServer(callee[2])
	m.WithCalleeService(callee[3])
}

// CalleeContainerName 被调方的容器名
func (m *msg) CalleeContainerName() string {
	return m.calleeContainerName
}

// WithCalleeContainerName 设置被调方的容器名
func (m *msg) WithCalleeContainerName(s string) {
	m.calleeContainerName = s
}

// CallerServiceName 主调方的服务名
func (m *msg) CallerServiceName() string {
	return m.callerServiceName
}

// WithCallerServiceName 设置主调方的服务名
func (m *msg) WithCallerServiceName(s string) {
	if m.callerServiceName == s {
		return
	}
	m.callerServiceName = s
	callers := strings.Split(s, ".") // trpc.app.server.service
	if len(callers) < ServiceSectionLength {
		return
	}
	m.WithCallerApp(callers[1])
	m.WithCallerServer(callers[2])
	m.WithCallerService(callers[3])
}

// ServerRspErr 服务端回包时设置的错误，一般为handler返回的err
func (m *msg) ServerRspErr() *errs.Error {
	if m.serverRspErr == nil {
		return nil
	}
	e, ok := m.serverRspErr.(*errs.Error)
	if !ok {
		return &errs.Error{
			Type: errs.ErrorTypeBusiness,
			Code: errs.RetUnknown,
			Msg:  m.serverRspErr.Error(),
		}
	}
	return e
}

// WithServerRspErr 设置服务端回包时设置的错误
func (m *msg) WithServerRspErr(e error) {
	m.serverRspErr = e
}

// ClientRspErr 客户端调用下游时返回的err
func (m *msg) ClientRspErr() error {
	return m.clientRspErr
}

// WithClientRspErr 设置客户端调用下游时返回的err 一般由客户端解析回包时调用
func (m *msg) WithClientRspErr(e error) {
	m.clientRspErr = e
}

// ServerReqHead 服务端收到请求的协议包头
func (m *msg) ServerReqHead() interface{} {
	return m.serverReqHead
}

// WithServerReqHead 设置服务端收到请求的协议包头
func (m *msg) WithServerReqHead(h interface{}) {
	m.serverReqHead = h
}

// ServerRspHead 服务端回给上游的协议包头
func (m *msg) ServerRspHead() interface{} {
	return m.serverRspHead
}

// WithServerRspHead 设置服务端回给上游的协议包头
func (m *msg) WithServerRspHead(h interface{}) {
	m.serverRspHead = h
}

// ClientReqHead 客户端请求下游设置的协议包头，一般不用设置，主要用于跨协议调用
func (m *msg) ClientReqHead() interface{} {
	return m.clientReqHead
}

// WithClientReqHead 设置客户端请求下游设置的协议包头
func (m *msg) WithClientReqHead(h interface{}) {
	m.clientReqHead = h
}

// ClientRspHead 下游回包的协议包头
func (m *msg) ClientRspHead() interface{} {
	return m.clientRspHead
}

// WithClientRspHead 设置下游回包的协议包头
func (m *msg) WithClientRspHead(h interface{}) {
	m.clientRspHead = h
}

// Dyeing 染色标记
func (m *msg) Dyeing() bool {
	return m.dyeing
}

// WithDyeing 设置染色标记
func (m *msg) WithDyeing(dyeing bool) {
	m.dyeing = dyeing
}

// DyeingKey 染色key
func (m *msg) DyeingKey() string {
	return m.dyeingKey
}

// WithDyeingKey 设置染色key
func (m *msg) WithDyeingKey(key string) {
	m.dyeingKey = key
}

// CallerApp 返回主调app
func (m *msg) CallerApp() string {
	return m.callerApp
}

// WithCallerApp 设置主调app
func (m *msg) WithCallerApp(app string) {
	m.callerApp = app
}

// CallerServer 返回主调server
func (m *msg) CallerServer() string {
	return m.callerServer
}

// WithCallerServer 设置主调app
func (m *msg) WithCallerServer(s string) {
	m.callerServer = s
}

// CallerService 返回主调service
func (m *msg) CallerService() string {
	return m.callerService
}

// WithCallerService 设置主调service
func (m *msg) WithCallerService(s string) {
	m.callerService = s
}

// WithCallerMethod 设置主调method
func (m *msg) WithCallerMethod(s string) {
	m.callerMethod = s
}

// CallerMehtod 返回主调method
func (m *msg) CallerMethod() string {
	return m.callerMethod
}

// CalleeApp 返回被调app
func (m *msg) CalleeApp() string {
	return m.calleeApp
}

// WithCalleeApp 设置被调app
func (m *msg) WithCalleeApp(app string) {
	m.calleeApp = app
}

// CalleeServer 返回被调server
func (m *msg) CalleeServer() string {
	return m.calleeServer
}

// WithCalleeServer 设置被调server
func (m *msg) WithCalleeServer(s string) {
	m.calleeServer = s
}

// CalleeService 返回被调service
func (m *msg) CalleeService() string {
	return m.calleeService
}

// WithCalleeService 设置被调service
func (m *msg) WithCalleeService(s string) {
	m.calleeService = s
}

// WithCalleeMethod 设置被调调method
func (m *msg) WithCalleeMethod(s string) {
	m.calleeMethod = s
}

// CalleeMehtod 返回被调method
func (m *msg) CalleeMethod() string {
	return m.calleeMethod
}

// WithLogger 设置日志logger到context msg里面，一般设置的是WithFields生成的新的logger
func (m *msg) WithLogger(l interface{}) {
	m.logger = l
}

// Logger 从msg取出logger
func (m *msg) Logger() interface{} {
	return m.logger
}

// WithNewMessage 创建新的空的message 放到ctx里面 server收到请求入口创建
func WithNewMessage(ctx context.Context) (context.Context, Msg) {

	m := msgPool.Get().(*msg)
	ctx = context.WithValue(ctx, ContextKeyMessage, m)
	m.context = ctx
	return ctx, m
}

// PutBackMessage return struct Message to sync pool,
// and reset all the members of Message to default
func PutBackMessage(sourceMsg Msg) {
	m, ok := sourceMsg.(*msg)
	if !ok {
		return
	}
	m.resetDefault()
	msgPool.Put(m)
}

// WithCloneMessage 复制一个新的message 放到ctx里面 每次rpc调用都必须创建新的msg 由client stub调用该函数
func WithCloneMessage(ctx context.Context) (context.Context, Msg) {

	newMsg := msgPool.Get().(*msg)

	val := ctx.Value(ContextKeyMessage)
	m, ok := val.(*msg)
	if !ok {
		ctx = context.WithValue(ctx, ContextKeyMessage, newMsg)
		newMsg.context = ctx
		return ctx, newMsg
	}

	ctx = context.WithValue(ctx, ContextKeyMessage, newMsg)
	newMsg.context = ctx

	newMsg.frameHead = m.frameHead
	newMsg.requestTimeout = m.requestTimeout
	newMsg.serializationType = m.serializationType
	newMsg.serverRPCName = m.serverRPCName
	newMsg.clientRPCName = m.clientRPCName
	newMsg.serverReqHead = m.serverReqHead
	newMsg.serverRspHead = m.serverRspHead
	newMsg.dyeing = m.dyeing
	newMsg.dyeingKey = m.dyeingKey
	newMsg.serverMetaData = m.serverMetaData
	newMsg.logger = m.logger

	newMsg.clientMetaData = m.serverMetaData.Clone()
	// clone是给下游client用的，所以caller等于callee
	newMsg.callerServiceName = m.calleeServiceName
	newMsg.callerApp = m.calleeApp
	newMsg.callerServer = m.calleeServer
	newMsg.callerService = m.calleeService
	newMsg.callerMethod = m.calleeMethod
	newMsg.namespace = m.namespace
	newMsg.envName = m.envName
	newMsg.setName = m.setName
	newMsg.envTransfer = m.envTransfer

	return ctx, newMsg
}

// Message 从ctx取出message
func Message(ctx context.Context) Msg {

	val := ctx.Value(ContextKeyMessage)
	m, ok := val.(*msg)
	if !ok {
		return &msg{context: ctx}
	}
	return m
}
