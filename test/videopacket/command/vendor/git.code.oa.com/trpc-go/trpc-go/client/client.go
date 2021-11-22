// Package client 客户端，包括网络通信 寻址路由 负载均衡 监控统计 链路跟踪
package client

import (
	"context"
	"fmt"
	"net"
	"strings"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/filter"
	"git.code.oa.com/trpc-go/trpc-go/metrics"
	"git.code.oa.com/trpc-go/trpc-go/naming/registry"
	"git.code.oa.com/trpc-go/trpc-go/naming/selector"
	"git.code.oa.com/trpc-go/trpc-go/transport"
)

// Client 客户端调用结构
type Client interface {
	// 发起后端调用
	Invoke(ctx context.Context, reqbody interface{}, rspbody interface{}, opt ...Option) error
}

// DefaultClient 默认的全局的唯一client 并发安全
var DefaultClient = New()

// New 创建一个client 使用全局默认client transport，也可以传参替换
var New = func() Client {
	return &client{}
}

type client struct{}

// getServiceInfoOptions 设置服务环境信息
func (c *client) getServiceInfoOptions(msg codec.Msg) []selector.Option {
	if len(msg.Namespace()) > 0 {
		return []selector.Option{
			selector.WithSourceNamespace(msg.Namespace()),
			selector.WithSourceServiceName(msg.CallerServiceName()),
			selector.WithSourceEnvName(msg.EnvName()),
			selector.WithEnvTransfer(msg.EnvTransfer()),
			selector.WithSourceSetName(msg.SetName()),
		}
	}

	return nil
}

// getOptions 获取每次请求所需的参数数据
func (c *client) getOptions(msg codec.Msg, opt ...Option) (*Options, error) {
	// 每次请求构造新的参数数据 保证并发安全
	opts := &Options{
		Transport:                transport.DefaultClientTransport,
		Selector:                 selector.DefaultSelector,
		endpoint:                 msg.CalleeServiceName(),
		SerializationType:        -1, // 初始值-1，不设置
		CurrentSerializationType: -1, // 当前client的序列化方式，协议里面的序列化方式以SerializationType为准，转发代理情况，
		CurrentCompressType:      -1, // 当前client透传body不序列化，但是业务协议后端需要指定序列化方式
	}

	// 以被调方协议文件的servicename(package.service)为key获取相关配置
	err := opts.LoadClientConfig(msg.CalleeServiceName())
	if err != nil {
		metrics.Counter("trpc.LoadClientConfigFail").Incr()
		return nil, err
	}

	// 设置服务环境信息
	opts.SelectOptions = append(opts.SelectOptions, c.getServiceInfoOptions(msg)...)

	// 输入参数为最高优先级 覆盖掉原有数据
	for _, o := range opt {
		o(opts)
	}

	err = opts.LoadClientFilterConfig(msg.CalleeServiceName())
	if err != nil {
		metrics.Counter("trpc.LoadClientFilterConfigFail").Incr()
		return nil, err
	}

	err = c.setNamingInfo(opts)
	if err != nil {
		return nil, err
	}

	return opts, nil
}

// setNamingInfo 设置名字服务相关信息
func (c *client) setNamingInfo(opts *Options) error {
	// 默认使用名字服务servicename获取地址，如果有指定target，则由指定的值来获取
	if opts.Target != "" {
		t := strings.SplitN(opts.Target, "://", 2)
		if len(t) != 2 {
			return errs.NewFrameError(errs.RetClientRouteErr, fmt.Sprintf("client: target %s schema invalid", opts.Target))
		}
		opts.Selector = selector.Get(t[0])
		// 检查selector是否为空
		if opts.Selector == nil {
			return errs.NewFrameError(errs.RetClientRouteErr, fmt.Sprintf("client: selector %s not exist", t[0]))
		}
		opts.endpoint = t[1]
	}

	return nil
}

func (c *client) getNode(opts *Options) (*registry.Node, error) {
	// 获取ipport请求地址
	node, err := opts.Selector.Select(opts.endpoint, opts.SelectOptions...)
	if err != nil {
		return nil, errs.NewFrameError(errs.RetClientRouteErr, "client Select: "+err.Error())
	}
	if node.Address == "" {
		return nil, errs.NewFrameError(errs.RetClientRouteErr, fmt.Sprintf("client Select: node address empty:%+v", node))
	}

	return node, nil
}

// selectNode 根据设置的寻址选择器寻址到后端节点,并设置msg
func (c *client) selectNode(msg codec.Msg, opts *Options) (*registry.Node, error) {

	node, err := c.getNode(opts)
	if err != nil {
		return nil, err
	}

	// 通过注册中心返回的节点配置信息更新设置参数
	opts.LoadNodeConfig(node)

	var addr net.Addr
	switch opts.Network {
	case "tcp", "tcp4", "tcp6":
		addr, _ = net.ResolveTCPAddr(node.Network, node.Address)
	case "udp", "udp4", "udp6":
		addr, _ = net.ResolveUDPAddr(node.Network, node.Address)
	default:
		addr, _ = net.ResolveTCPAddr("tcp4", node.Address)
	}
	msg.WithRemoteAddr(addr)
	msg.WithCalleeContainerName(node.ContainerName)

	if len(msg.EnvTransfer()) > 0 {
		msg.WithEnvTransfer(msg.EnvTransfer())
	} else {
		msg.WithEnvTransfer(node.EnvKey)
	}

	return node, nil
}

// getMetaData 获取后端透传参数
func (c *client) getMetaData(msg codec.Msg, opts *Options) codec.MetaData {
	md := msg.ClientMetaData()
	if md == nil {
		md = codec.MetaData{}
	}
	for k, v := range opts.MetaData {
		md[k] = v
	}

	return md
}

// updateMsg 更新客户端请求Msg上下文信息
func (c *client) updateMsg(msg codec.Msg, opts *Options) {

	// 设置被调方service name 一般service name和proto协议的package.service一致，但是用户可以通过参数修改
	if len(opts.ServiceName) > 0 {
		msg.WithCalleeServiceName(opts.ServiceName) // 以client角度看，caller是自身，callee是下游
	}

	if len(opts.CalleeMethod) > 0 {
		msg.WithCalleeMethod(opts.CalleeMethod)
	}

	// 设置后端透传参数
	msg.WithClientMetaData(c.getMetaData(msg, opts))

	// 以client作为小工具时，没有caller，需要自己通过client option设置进来
	if len(opts.CallerServiceName) > 0 {
		msg.WithCallerServiceName(opts.CallerServiceName)
	}
	if opts.SerializationType >= 0 {
		msg.WithSerializationType(opts.SerializationType)
	}
	if opts.CompressType > 0 {
		msg.WithCompressType(opts.CompressType)
	}

	// 用户设置reqhead，希望使用自己的请求包头
	if opts.ReqHead != nil {
		msg.WithClientReqHead(opts.ReqHead)
	}
	// 用户设置rsphead，希望回传后端的响应包头
	if opts.RspHead != nil {
		msg.WithClientRspHead(opts.RspHead)
	}
}

// Invoke 启动后端调用，传入具体业务协议结构体 内部调用codec transport
func (c *client) Invoke(ctx context.Context, reqbody interface{}, rspbody interface{}, opt ...Option) error {

	// 取出当前请求链路的通用消息结构数据, 每个client后端调用都是新的msg，由client stub创建生成
	msg := codec.Message(ctx)

	// 读取配置参数，设置用户输入参数
	opts, err := c.getOptions(msg, opt...)
	if err != nil {
		return err
	}

	// 根据寻址选择器寻址到后端节点node
	node, err := c.selectNode(msg, opts)
	if err != nil {
		metrics.Counter("trpc.SelectNodeFail").Incr()
		return err
	}

	begin := time.Now()
	defer func() {
		// 计算duration使用time.Since(begin)要优于time.Now().Sub(begin),
		// 因为go1.12之后time.Since对于有nano_time的time对象有fast path处理: 直接使用runtime层的nano time计算, 可减少一次系统调用, 提升性能45%
		// 见: https://github.com/golang/go/commit/fc3f8d43f1b7da3ee3fb9a5181f2a86841620273
		cost := time.Since(begin)
		if e, ok := err.(*errs.Error); ok &&
			e.Type == errs.ErrorTypeFramework &&
			(e.Code == errs.RetClientConnectFail ||
				e.Code == errs.RetClientTimeout ||
				e.Code == errs.RetClientNetErr) {
			e.Msg = fmt.Sprintf("%s, cost:%s", e.Msg, cost)
			opts.Selector.Report(node, cost, err)
		} else {
			opts.Selector.Report(node, cost, nil)
		}
		if opts.Node != nil { //将node信息回传给用户
			opts.Node.ServiceName = node.ServiceName
			opts.Node.Address = node.Address
			opts.Node.CostTime = cost
		}
	}()

	// 更新设置参数后，检查codec是否为空
	if opts.Codec == nil {
		metrics.Counter("trpc.ClientCodecEmpty").Incr()
		return errs.NewFrameError(errs.RetClientEncodeFail, "client: codec empty")
	}

	// 根据获取的Opts信息更新Msg
	c.updateMsg(msg, opts)

	// 设置当前后端请求超时
	if opts.Timeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, opts.Timeout)
		defer cancel()
	}

	// 设置允许下游超时的时间
	deadline, ok := ctx.Deadline()
	if ok {
		msg.WithRequestTimeout(deadline.Sub(begin))
	}

	// 调用拦截器
	if opts.DisableFilter {
		opts.Filters = nil
	}
	err = opts.Filters.Handle(ctx, reqbody, rspbody, c.callFunc(msg, opts))
	if err != nil {
		return err
	}

	return nil
}

func prepareRequestBuf(msg codec.Msg, reqbody interface{}, opts *Options) ([]byte, error) {

	serializationType := msg.SerializationType()
	compressType := msg.CompressType()
	if opts.CurrentSerializationType >= 0 {
		serializationType = opts.CurrentSerializationType
	}
	if opts.CurrentCompressType >= 0 {
		compressType = opts.CurrentCompressType
	}

	var err error
	// 序列化具体body结构体到二进制body
	var reqbodybuf []byte
	if serializationType > -1 {
		reqbodybuf, err = codec.Marshal(serializationType, reqbody)
		if err != nil {
			return nil, errs.NewFrameError(errs.RetClientEncodeFail, "client codec Marshal: "+err.Error())
		}
	}

	// 压缩
	if compressType > 0 {
		reqbodybuf, err = codec.Compress(compressType, reqbodybuf)
		if err != nil {
			return nil, errs.NewFrameError(errs.RetClientEncodeFail, "client codec Compress: "+err.Error())
		}
	}

	// 打包整个请求buffer
	reqbuf, err := opts.Codec.Encode(msg, reqbodybuf)
	if err != nil {
		return nil, errs.NewFrameError(errs.RetClientEncodeFail, "client codec Encode: "+err.Error())
	}

	return reqbuf, nil
}

func processResponseBuf(msg codec.Msg, rspbody interface{}, rspbodybuf []byte, opts *Options) error {
	// 后端响应业务错误码
	if msg.ClientRspErr() != nil {
		return msg.ClientRspErr()
	}

	if len(rspbodybuf) == 0 {
		return nil
	}

	// 允许后端回包修改 serialization type, compress type
	serializationType := msg.SerializationType()
	compressType := msg.CompressType()
	if opts.CurrentSerializationType >= 0 {
		serializationType = opts.CurrentSerializationType
	}
	if opts.CurrentCompressType >= 0 {
		compressType = opts.CurrentCompressType
	}

	var err error
	// 解压缩
	if compressType > 0 {
		rspbodybuf, err = codec.Decompress(compressType, rspbodybuf)
		if err != nil {
			return errs.NewFrameError(errs.RetClientDecodeFail, "client codec Decompress: "+err.Error())
		}
	}

	// 反序列化二进制body到具体body结构体
	if serializationType > -1 {
		err = codec.Unmarshal(serializationType, rspbodybuf, rspbody)
		if err != nil {
			return errs.NewFrameError(errs.RetClientDecodeFail, "client codec Unmarshal: "+err.Error())
		}
	}

	return nil
}

// callFunc 后端调用函数，内部包括codec打解包，transport网络通信，以该函数前后为拦截器入口
func (c *client) callFunc(msg codec.Msg, opts *Options) filter.HandleFunc {

	return func(ctx context.Context, reqbody interface{}, rspbody interface{}) (err error) {

		reqbuf, err := prepareRequestBuf(msg, reqbody, opts)
		if err != nil {
			return err
		}

		// 发起后端网络请求
		rspbuf, err := opts.Transport.RoundTrip(ctx, reqbuf, opts.CallOptions...)
		if err != nil {
			if err == errs.ErrClientNoResponse { // 只发不收模式，没有回包，返回成功
				return nil
			}
			return err
		}

		// 解包整个响应buffer
		rspbodybuf, err := opts.Codec.Decode(msg, rspbuf)
		if err != nil {
			return errs.NewFrameError(errs.RetClientDecodeFail, "client codec Decode: "+err.Error())
		}

		return processResponseBuf(msg, rspbody, rspbodybuf, opts)
	}
}
