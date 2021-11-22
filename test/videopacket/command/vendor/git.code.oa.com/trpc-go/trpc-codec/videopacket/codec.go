package videopacket

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"sync/atomic"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/transport"
	"git.code.oa.com/trpc-go/trpc-utils/copyutils"
	p "git.code.oa.com/videocommlib/videopacket-go"
)

var (
	_ codec.Codec = &ServerCodec{}
	_ codec.Codec = &ClientCodec{}
)
var (
	DefaultClientCodec   = &ClientCodec{}
	DefaultServerCodec   = &ServerCodec{}
	DefaultFramerBuilder = &FramerBuilder{}

	// 是否强制开启命令字模式
	forceEnableCommand = false
)

func init() {
	codec.Register("videopacket", &ServerCodec{}, &ClientCodec{})
	transport.RegisterFramerBuilder("videopacket", &FramerBuilder{})
}

// ForceEnableCommand 用于开启命令字模式
// 由于历史原因，videopacket 的函数路由支持两种模式，命令字与非命令字模式
// 区分两种的方式为判断 head.CommHeader.BasicInfo.Command 是否为0，非0则为命令字模式，其他为按照 ToServerName 和 FuncName 来路由
// 但是已有业务使用命令字 0 来路由，导致与其他团队冲突，故使用 forceEnableCommand 标记是否强制使用命令字模式
// 如果强制使用则 0x0 的命令字也可以通过命令字路由
func ForceEnableCommand() {
	forceEnableCommand = true
}

var (
	// ErrWrongHead is the error videopacket wrong head
	ErrWrongHead = errors.New("videopacket wrong head")
	// ErrWrongLength is the error videopacket wrong length
	ErrWrongLength = errors.New("videopacket wrong length")
	// ErrNoRPCName is the error when request no rpc name
	ErrNoRPCName = errors.New("no rpc name")
	// ErrClientReqHeadWrongType is the error when client req setted but wrong type
	ErrClientReqHeadWrongType = errors.New("client req head wrong type")
)

var (
	defaultMsgSize     = 4096
	frameHeadLength    = 16
	frameTailLength    = 1
	defaulMaxFrameSize = 2 * 1024 * 1024 // 默认最大帧为 2M
	// videopacketInnerMaxErrorCode 为 videopacket 预留的最大错误码
	// videopacket 回包时错误码放到包头的 CommHeader.BasicInfo.Result 中
	// 如果是业务错误码，此时 CommHeader.BasicInfo.Result 需大于 1000
	videopacketInnerMaxErrorCode int32 = 1000
)

// FramerBuilder is videopacket framer builder
type FramerBuilder struct{}

// New 返回一个 transport.Framer 的实现
func (fb *FramerBuilder) New(reader io.Reader) transport.Framer {
	return newFramer(reader)
}

func newFramer(r io.Reader) transport.Framer {
	return &framer{
		reader: r,
		msg:    make([]byte, 0, defaultMsgSize),
	}
}

type framer struct {
	reader io.Reader
	/*
		start      byte
		length     uint32 => 4byte
		version    byte
		reserve    [10]byte
	*/
	head [16]byte
	msg  []byte
	end  byte
}

// ReadFrame 从 io.Reader 拆分出完整的数据帧
func (f *framer) ReadFrame() ([]byte, error) {
	length, err := f.readFrameHead()
	if err != nil {
		return nil, err
	}

	return f.readFrameBody(length)
}

// readFrameHead 设置framer head
func (f *framer) readFrameHead() (length uint32, err error) {
	num, err := io.ReadFull(f.reader, f.head[:frameHeadLength])
	if err != nil {
		return
	}
	if num != frameHeadLength {
		return length, errors.New("videopacket framer: read framer head num invalid")
	}
	if f.head[0] != p.VideopacketStart {
		return length, errors.New("videopacket framer: wrong header start")
	}
	length = binary.BigEndian.Uint32(f.head[1:5])
	if length < p.VideopacketMinLength {
		return length, errors.New("videopacket framer: length less than min packet length")
	}

	if length > uint32(defaulMaxFrameSize) {
		err = errors.New("videopacket framer: read frame header total len too large")
	}
	return
}

// readFrameBody 设置framer body
func (f *framer) readFrameBody(length uint32) ([]byte, error) {
	if int(length) > len(f.msg) {
		f.msg = make([]byte, length)
	}
	copy(f.msg, f.head[:])

	num, err := io.ReadFull(f.reader, f.msg[frameHeadLength:length])
	if err != nil {
		return nil, err
	}
	if num != int(length)-frameHeadLength {
		return nil, errors.New("videopacket framer: read frame total num invalid")
	}
	return f.msg[:length], nil
}

// ServerCodec 为服务端编解码的实现
type ServerCodec struct{}

// Encode 服务端编码
func (s *ServerCodec) Encode(message codec.Msg, body []byte) (buffer []byte, err error) {
	head, ok := message.ServerRspHead().(*p.VideoPacket)
	if !ok {
		head = p.NewVideoPacket()
	}

	setRspHeadErr(message, head)
	message.WithServerReqHead(head)

	head.CommHeader.Body = string(body)
	bf, err := head.Encode()
	if err != nil {
		return nil, fmt.Errorf("videopacket server encode with err: %s", err.Error())
	}
	if needBodyPack(head) {
		message.WithSerializationType(VideoPacketBodyPackSerializationType)
	} else {
		message.WithSerializationType(codec.SerializationTypeJCE)
	}
	return bf.Bytes(), nil
}

// setRspHeadErr set err
func setRspHeadErr(message codec.Msg, head *p.VideoPacket) {
	// message.ServerRspErr() != nil 时转化为头部错误码
	// 业务错误码直接赋值，框架错误码由于 videopacket 错误码 1000 内预留，故 +1000
	if err := message.ServerRspErr(); err != nil {
		if err.Type == errs.ErrorTypeBusiness {
			head.CommHeader.BasicInfo.Result = err.Code
		} else {
			head.CommHeader.BasicInfo.Result = videopacketInnerMaxErrorCode + err.Code
		}
	}
}

// Decode 服务端解码
func (s *ServerCodec) Decode(message codec.Msg, buffer []byte) (body []byte, err error) {
	if len(buffer) < p.VideopacketMinLength {
		return nil, fmt.Errorf("videopacket server decode with length: %d less than min length: %d",
			len(buffer), p.VideopacketMinLength)
	}

	// 读取包头
	head := p.NewVideoPacket()
	if err := head.Decode(buffer); err != nil {
		return nil, fmt.Errorf("videopacket server decode with err: %s", err.Error())
	}
	reqBody := []byte(head.CommHeader.Body)

	// body 设置为空
	head.CommHeader.Body = ""
	message.WithServerReqHead(head)

	setRspHead(message, head)
	setRPCName(message, head)

	// 设置 body 的序列化方式
	if needBodyPack(head) {
		message.WithSerializationType(VideoPacketBodyPackSerializationType)
	} else {
		message.WithSerializationType(codec.SerializationTypeJCE)
	}

	return reqBody, nil
}

// setRspHead set response head
func setRspHead(message codec.Msg, head *p.VideoPacket) {
	// 提前构造响应包头
	serverRspHead := *head
	message.WithServerRspHead(&serverRspHead)

	message.WithCallerServiceName(head.CommHeader.ServerRoute.FromServerName)
	message.WithCalleeServiceName(head.CommHeader.ServerRoute.ToServerName)
	message.WithCallerMethod(head.CommHeader.ServerRoute.FromFuncName)
	message.WithCalleeMethod(head.CommHeader.ServerRoute.FuncName)
}

// setRPCName 设置 rpc name
func setRPCName(message codec.Msg, head *p.VideoPacket) {
	// forceEnableCommand 或命令字非 0 时使用命令字
	// 否则使用被请求服务的名字和被请求服务的函数名字
	// 都为空时返回错误
	var rpcName string
	if forceEnableCommand || head.CommHeader.BasicInfo.Command != 0 {
		rpcName = fmt.Sprintf("cmd_%#x", uint16(head.CommHeader.BasicInfo.Command))
	} else if head.CommHeader.ServerRoute.ToServerName != "" &&
		head.CommHeader.ServerRoute.FuncName != "" {
		rpcName = fmt.Sprintf("%s/%s",
			head.CommHeader.ServerRoute.ToServerName, head.CommHeader.ServerRoute.FuncName)
	}
	message.WithServerRPCName(rpcName)
}

// ClientCodec 为客户端编解码的实现
type ClientCodec struct {
	RequestID uint32
}

// Encode 客户端编码
func (c *ClientCodec) Encode(message codec.Msg, body []byte) (buffer []byte, err error) {
	req, err := c.getRequestHead(message)
	if err != nil {
		return nil, err
	}

	//框架自动生成全局唯一seq
	if req.CommHeader.BasicInfo.SeqId == 0 {
		req.CommHeader.BasicInfo.SeqId = int64(atomic.AddUint32(&c.RequestID, 1))
	}

	message.WithClientReqHead(req)

	setCliReqHead(message, req)

	req.CommHeader.Body = string(body)
	bf, err := req.Encode()
	if err != nil {
		return nil, fmt.Errorf("videopacket client encode with err: %s", err.Error())
	}
	bs := bf.Bytes()
	return bs, nil
}

// setCliReqHead set client request head
func setCliReqHead(message codec.Msg, req *p.VideoPacket) {
	//设置被调方 service name
	if req.CommHeader.ServerRoute.ToServerName != "" {
		message.WithCalleeServer(req.CommHeader.ServerRoute.ToServerName)
	}
	message.WithCalleeMethod(req.CommHeader.ServerRoute.FuncName)

	if message.CalleeMethod() == "" {
		message.WithCalleeMethod(message.ClientRPCName())
	}
}

func (c *ClientCodec) getRequestHead(msg codec.Msg) (*p.VideoPacket, error) {
	if msg.ClientReqHead() != nil {
		// client req head不为空 说明是用户自己创建，直接使用即可
		return getCliReqHead(msg)
	}

	req := p.NewVideoPacket()
	if serverReq, ok := msg.ServerReqHead().(*p.VideoPacket); ok {
		if copiedPacket, err := copyutils.DeepCopy(serverReq); err != nil {
			return nil, err
		} else {
			if req, ok = copiedPacket.(*p.VideoPacket); !ok {
				return nil, errors.New("copiedPacket is not *p.VideoPacket")
			}
		}
	}
	return req, nil
}

// getCliReqHead get reqhead from msg
func getCliReqHead(msg codec.Msg) (*p.VideoPacket, error) {
	req, ok := msg.ClientReqHead().(*p.VideoPacket)
	if !ok {
		return nil, ErrClientReqHeadWrongType
	}
	return req, nil
}

// Decode 客户端解码
func (c *ClientCodec) Decode(message codec.Msg, body []byte) (buffer []byte, err error) {
	if len(body) < p.VideopacketMinLength {
		return nil, fmt.Errorf("videopacket client decode with length: %d less than min length: %d",
			len(body), p.VideopacketMinLength)
	}

	rsp, err := getCliRspHead(message)
	if err != nil {
		return nil, err
	}

	if err = rsp.Decode(body); err != nil {
		return nil, fmt.Errorf("videopacket client decode with err: %s", err.Error())
	}

	message.WithClientRspHead(rsp)
	if videopacketErrCode := rsp.CommHeader.BasicInfo.Result; videopacketErrCode != 0 {
		if videopacketErrCode > videopacketInnerMaxErrorCode {
			message.WithClientRspErr(&errs.Error{
				Type: errs.ErrorTypeCalleeFramework,
				Code: videopacketErrCode - videopacketInnerMaxErrorCode,
				Desc: "videopacket bussiness error",
				Msg:  fmt.Sprintf("videopacket response commHeader.BasicInfo.Result is %d", videopacketErrCode),
			})
		} else {
			message.WithClientRspErr(errs.New(int(videopacketErrCode),
				fmt.Sprintf("videopacket response commHeader.BasicInfo.Result is %d", videopacketErrCode)))
		}
	}
	return []byte(rsp.CommHeader.Body), nil
}

// getClientRspHead 构造后端响应包头
func getCliRspHead(message codec.Msg) (rsp *p.VideoPacket, err error) {
	if message.ClientRspHead() != nil {
		//client rsp head不为空 说明是用户故意创建，希望底层回传后端响应包头
		response, ok := message.ClientRspHead().(*p.VideoPacket)
		if !ok {
			return nil, ErrClientReqHeadWrongType
		}
		rsp = response
	} else {
		//client rsp head为空 说明用户不关心后端响应包头
		rsp = p.NewVideoPacket()
	}
	return
}
