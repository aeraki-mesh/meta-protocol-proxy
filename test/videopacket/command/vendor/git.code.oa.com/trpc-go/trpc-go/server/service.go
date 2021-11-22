package server

import (
	"context"
	"errors"
	"fmt"
	"reflect"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/filter"
	"git.code.oa.com/trpc-go/trpc-go/log"
	"git.code.oa.com/trpc-go/trpc-go/metrics"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
	"git.code.oa.com/trpc-go/trpc-go/transport"
)

// Service 服务端调用结构
type Service interface {
	// 注册路由信息
	Register(serviceDesc interface{}, serviceImpl interface{}) error
	// 启动服务
	Serve() error
	// 关闭服务
	Close(chan struct{}) error
}

// FilterFunc 内部解析reqbody 并返回拦截器 传给server stub
type FilterFunc func(reqbody interface{}) (filter.Chain, error)

// Method 服务rpc方法信息
type Method struct {
	Name string
	Func func(svr interface{}, ctx context.Context, f FilterFunc) (rspbody interface{}, err error)
}

// ServiceDesc 服务描述service定义
type ServiceDesc struct {
	ServiceName string
	HandlerType interface{}
	Methods     []Method
}

// Handler trpc默认的handler
type Handler func(ctx context.Context, f FilterFunc) (rspbody interface{}, err error)

// service Service实现
type service struct {
	ctx    context.Context    // service关闭
	cancel context.CancelFunc // service关闭

	opts *Options // service选项

	handlers map[string]Handler // rpcname => handler
}

// New 创建一个service 使用全局默认server transport，也可以传参替换
var New = func(opts ...Option) Service {

	s := &service{
		opts: &Options{
			protocol:                 "unknown-protocol",
			ServiceName:              "empty-name",
			CurrentSerializationType: -1,
			CurrentCompressType:      -1,
			Transport:                transport.DefaultServerTransport,
		},
		handlers: make(map[string]Handler),
	}

	for _, o := range opts {
		o(s.opts)
	}

	if !s.opts.handlerSet { // 没有设置handler 则将该server作为transport的handler
		s.opts.ServeOptions = append(s.opts.ServeOptions, transport.WithHandler(s))
	}

	s.ctx, s.cancel = context.WithCancel(context.Background())

	return s
}

// Serve 启动服务
func (s *service) Serve() (err error) {

	// 确保正常监听之后才能启动服务注册
	if err = s.opts.Transport.ListenAndServe(s.ctx, s.opts.ServeOptions...); err != nil {
		log.Errorf("service:%s ListenAndServe fail:%v", s.opts.ServiceName, err)
		return err
	}

	if s.opts.Registry != nil {
		err = s.opts.Registry.Register(s.opts.ServiceName, registry.WithAddress(s.opts.Address))
		if err != nil {
			// 有注册失败，关闭service，并返回给上层错误
			log.Errorf("service:%s register fail:%v", s.opts.ServiceName, err)
			return err
		}
	}

	log.Infof("%s service:%s launch success, address:%s, serving ...", s.opts.protocol, s.opts.ServiceName, s.opts.Address)

	metrics.Counter("trpc.ServiceStart").Incr()
	<-s.ctx.Done()
	return nil
}

// Handle server transport收到请求包后调用此函数
func (s *service) Handle(ctx context.Context, reqbuf []byte) (rspbuf []byte, err error) {

	// 无法回包，只能丢弃
	if s.opts.Codec == nil {
		log.ErrorContextf(ctx, "server codec empty")
		metrics.Counter("trpc.ServerCodecEmpty").Incr()
		return nil, errors.New("server codec empty")
	}

	var rspbodybuf []byte

	msg := codec.Message(ctx)

	rspbody, err := s.handle(ctx, msg, reqbuf)
	if err != nil {
		// 不回包
		if err == errs.ErrServerNoResponse {
			return nil, err
		}
		// 处理失败 给客户端返回错误码, 忽略rspbody
		metrics.Counter("trpc.ServiceHandleFail").Incr()
		return s.encode(ctx, msg, rspbodybuf, err)
	}

	// 默认以协议字段的序列化为准，当有设置option则以option为准
	serializationType := msg.SerializationType()
	compressType := msg.CompressType()
	if s.opts.CurrentSerializationType >= 0 {
		serializationType = s.opts.CurrentSerializationType
	}
	if s.opts.CurrentCompressType >= 0 {
		compressType = s.opts.CurrentCompressType
	}

	// 业务处理成功 才开始打包body
	rspbodybuf, err = codec.Marshal(serializationType, rspbody)
	if err != nil {
		metrics.Counter("trpc.ServiceCodecMarshalFail").Incr()
		err = errs.NewFrameError(errs.RetServerEncodeFail, "service codec Marshal: "+err.Error())
		// 处理失败 给客户端返回错误码
		return s.encode(ctx, msg, rspbodybuf, err)
	}

	// 处理成功 才开始压缩body
	rspbodybuf, err = codec.Compress(compressType, rspbodybuf)
	if err != nil {
		metrics.Counter("trpc.ServiceCodecCompressFail").Incr()
		err = errs.NewFrameError(errs.RetServerEncodeFail, "service codec Compress: "+err.Error())
		// 处理失败 给客户端返回错误码
		return s.encode(ctx, msg, rspbodybuf, err)
	}

	return s.encode(ctx, msg, rspbodybuf, nil)
}

func (s *service) encode(ctx context.Context, msg codec.Msg, rspbodybuf []byte, e error) (rspbuf []byte, err error) {

	if e != nil {
		msg.WithServerRspErr(e)
	}

	rspbuf, err = s.opts.Codec.Encode(msg, rspbodybuf)
	if err != nil {
		metrics.Counter("trpc.ServiceCodecEncodeFail").Incr()
		log.ErrorContextf(ctx, "service:%s encode fail:%v", s.opts.ServiceName, err)
		return nil, err
	}

	return rspbuf, nil
}

func (s *service) handle(ctx context.Context, msg codec.Msg, reqbuf []byte) (rspbody interface{}, err error) {

	msg.WithNamespace(s.opts.Namespace)           // server 的命名空间
	msg.WithEnvName(s.opts.EnvName)               // server 的环境
	msg.WithSetName(s.opts.SetName)               // server 的set
	msg.WithCalleeServiceName(s.opts.ServiceName) // 以server角度看，caller是上游，callee是自身

	reqbodybuf, err := s.opts.Codec.Decode(msg, reqbuf)
	if err != nil {
		metrics.Counter("trpc.ServiceCodecDecodeFail").Incr()
		return nil, errs.NewFrameError(errs.RetServerDecodeFail, "service codec Decode: "+err.Error())
	}

	// 再赋值一遍，防止decode更改了
	msg.WithNamespace(s.opts.Namespace)           // server 的命名空间
	msg.WithEnvName(s.opts.EnvName)               // server 的环境
	msg.WithSetName(s.opts.SetName)               // server 的set
	msg.WithCalleeServiceName(s.opts.ServiceName) // 以server角度看，caller是上游，callee是自身

	handler, ok := s.handlers[msg.ServerRPCName()]
	if !ok {
		handler, ok = s.handlers["*"] // 支持通配符全匹配转发处理
		if !ok {
			metrics.Counter("trpc.ServiceHandleRpcNameInvalid").Incr()
			return nil, errs.NewFrameError(errs.RetServerNoFunc,
				fmt.Sprintf("service handle: rpc name %s invalid, current service:%s",
					msg.ServerRPCName(), msg.CalleeServiceName()))
		}
	}

	timeout := s.opts.Timeout
	if msg.RequestTimeout() > 0 && !s.opts.DisableRequestTimeout { // 可以配置禁用
		if msg.RequestTimeout() < timeout || timeout == 0 { // 取最小值
			timeout = msg.RequestTimeout()
		}
	}
	if timeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, timeout)
		defer cancel()
	}

	return handler(ctx, s.filterFunc(msg, reqbodybuf))
}

// filterFunc 生成service拦截器函数 传给server stub，由生成代码来调用该拦截器，以具体业务处理handler前后为拦截入口
func (s *service) filterFunc(msg codec.Msg, reqbodybuf []byte) FilterFunc {

	// 将解压缩，序列化放到该闭包函数内部，允许生成代码里面修改解压缩方式和序列化方式，用于代理层透传
	return func(reqbody interface{}) (filter.Chain, error) {

		// 默认以协议字段的序列化为准，当有设置option则以option为准
		serializationType := msg.SerializationType()
		compressType := msg.CompressType()
		if s.opts.CurrentSerializationType >= 0 {
			serializationType = s.opts.CurrentSerializationType
		}
		if s.opts.CurrentCompressType >= 0 {
			compressType = s.opts.CurrentCompressType
		}

		// 解压body
		reqbodybuf, err := codec.Decompress(compressType, reqbodybuf)
		if err != nil {
			metrics.Counter("trpc.ServiceCodecDecompressFail").Incr()
			return nil, errs.NewFrameError(errs.RetServerDecodeFail, "service codec Decompress: "+err.Error())
		}

		// 反序列化body
		err = codec.Unmarshal(serializationType, reqbodybuf, reqbody)
		if err != nil {
			metrics.Counter("trpc.ServiceCodecUnmarshalFail").Incr()
			return nil, errs.NewFrameError(errs.RetServerDecodeFail, "service codec Unmarshal: "+err.Error())
		}

		return s.opts.Filters, nil
	}
}

// Register 把service业务实现接口注册到server里面
func (s *service) Register(serviceDesc interface{}, serviceImpl interface{}) error {

	desc, ok := serviceDesc.(*ServiceDesc)
	if !ok {
		return errors.New("serviceDesc is not *ServiceDesc")
	}

	if serviceImpl != nil {
		ht := reflect.TypeOf(desc.HandlerType).Elem()
		hi := reflect.TypeOf(serviceImpl)
		if !hi.Implements(ht) {
			return fmt.Errorf("%s not implements interface %s", hi.String(), ht.String())
		}
	}

	for _, method := range desc.Methods {
		h := method.Func
		s.handlers[method.Name] = func(ctx context.Context, f FilterFunc) (rsp interface{}, err error) {
			return h(serviceImpl, ctx, f)
		}
	}

	return nil
}

// Close service关闭动作，service取消registry注册
func (s *service) Close(ch chan struct{}) error {

	if ch == nil {
		ch = make(chan struct{}, 1)
	}

	log.Infof("%s service:%s, closing ...", s.opts.protocol, s.opts.ServiceName)

	if s.opts.Registry != nil {
		err := s.opts.Registry.Deregister(s.opts.ServiceName)
		if err != nil {
			log.Errorf("deregister service:%s fail:%v", s.opts.ServiceName, err)
		}
	}

	// service关闭，通知派生的下游ctx全部取消
	s.cancel()
	timeout := time.Millisecond * 300
	if s.opts.Timeout > timeout { // 取最大值
		timeout = s.opts.Timeout
	}
	time.Sleep(timeout)

	log.Infof("%s service:%s, closed", s.opts.protocol, s.opts.ServiceName)
	ch <- struct{}{}
	return nil
}
