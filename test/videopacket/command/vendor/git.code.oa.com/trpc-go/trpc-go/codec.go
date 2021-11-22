package trpc

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"os"
	"path"
	"sync/atomic"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/codec"
	"git.code.oa.com/trpc-go/trpc-go/errs"
	"git.code.oa.com/trpc-go/trpc-go/transport"

	"github.com/golang/protobuf/proto"
)

func init() {
	codec.Register("trpc", DefaultServerCodec, DefaultClientCodec)
	transport.RegisterFramerBuilder("trpc", DefaultFramerBuilder)
}

// default codec
var (
	DefaultServerCodec   = &ServerCodec{}
	DefaultClientCodec   = &ClientCodec{}
	DefaultFramerBuilder = &FramerBuilder{}
)

// DefaultMaxFrameSize 默认帧最大10M
var DefaultMaxFrameSize = 10 * 1024 * 1024

// trpc protocol codec
// 具体协议格式参考：https://git.code.oa.com/trpc/trpc-protocol/blob/master/docs/protocol_design.md
const (
	// 起始魔数
	stx = uint16(TrpcMagic_TRPC_MAGIC_VALUE)
	// 帧头 2 bytes stx + 1 byte type + 1 byte status + 4 bytes total len
	// + 2 bytes pb header len + 2 bytes stream id + 4 bytes reserved
	frameHeadLen = uint16(16)

	DyeingKey   = "trpc-dyeing-key" // 染色key
	UserIP      = "trpc-user-ip"    // 客户端最上游ip
	EnvTransfer = "trpc-env"        // 透传环境数据
)

// FrameHead trpc 桢头信息
type FrameHead struct {
	FrameType     uint8
	FrameStatus   uint8
	FrameReserved uint32
	StreamID      uint16
}

const defaultMsgSize = 4096

// FramerBuilder 数据帧构造器
type FramerBuilder struct{}

// New 生成一个trpc数据帧
func (fb *FramerBuilder) New(reader io.Reader) codec.Framer {
	return &framer{
		msg:    make([]byte, defaultMsgSize),
		reader: reader,
	}
}

type framer struct {
	reader io.Reader
	msg    []byte
}

// ReadFrame 从io reader拆分出完整数据桢
func (f *framer) ReadFrame() (msgbuf []byte, err error) {
	num, err := io.ReadFull(f.reader, f.msg[:frameHeadLen])
	if err != nil {
		return nil, err
	}
	if num != int(frameHeadLen) {
		return nil, errors.New("trpc framer: read frame header num invalid")
	}

	magic := binary.BigEndian.Uint16(f.msg[:2])
	if magic != uint16(TrpcMagic_TRPC_MAGIC_VALUE) {
		return nil, errors.New("trpc framer: read framer head magic not match")
	}

	totalLen := binary.BigEndian.Uint32(f.msg[4:8])
	if totalLen <= uint32(frameHeadLen) {
		return nil, errors.New("trpc framer: read frame header total len invalid")
	}

	if totalLen > uint32(DefaultMaxFrameSize) {
		return nil, errors.New("trpc framer: read frame header total len too large")
	}

	if int(totalLen) > len(f.msg) {
		head := f.msg[:frameHeadLen]
		f.msg = make([]byte, totalLen)
		copy(f.msg, head[:])
	}

	num, err = io.ReadFull(f.reader, f.msg[frameHeadLen:totalLen])
	if err != nil {
		return nil, err
	}
	if num != int(totalLen-uint32(frameHeadLen)) {
		return nil, errors.New("trpc framer: read frame total num invalid")
	}

	return f.msg[:totalLen], nil
}

// ServerCodec trpc服务端编解码
type ServerCodec struct {
}

// Decode 服务端收到客户端二进制请求数据解包到reqbody, service handler会自动创建一个新的空的msg 作为初始通用消息体
func (s *ServerCodec) Decode(msg codec.Msg, reqbuf []byte) ([]byte, error) {
	req := &RequestProtocol{}

	if len(reqbuf) < int(frameHeadLen) {
		return nil, errors.New("server decode req buf len invalid")
	}

	pbHeadLen := binary.BigEndian.Uint16(reqbuf[8:10])
	if pbHeadLen == 0 {
		return nil, errors.New("server decode pb head len empty")
	}
	if int(pbHeadLen) > len(reqbuf)-int(frameHeadLen) {
		return nil, errors.New("server decode pb head len invalid")
	}
	if err := proto.Unmarshal(reqbuf[frameHeadLen:frameHeadLen+pbHeadLen], req); err != nil {
		return nil, err
	}

	frameHead := &FrameHead{
		FrameType:     reqbuf[2],
		FrameStatus:   reqbuf[3],
		StreamID:      binary.BigEndian.Uint16(reqbuf[10:12]),
		FrameReserved: binary.BigEndian.Uint32(reqbuf[12:16]),
	}

	// 解请求包体
	reqbody := reqbuf[frameHeadLen+pbHeadLen:]

	// 提前构造响应包头
	rsp := &ResponseProtocol{
		Version:         uint32(TrpcProtoVersion_TRPC_PROTO_V1),
		CallType:        req.CallType,
		RequestId:       req.RequestId,
		MessageType:     req.MessageType,
		ContentType:     req.ContentType,
		ContentEncoding: req.ContentEncoding,
	}

	s.updateMsg(msg, frameHead, req, rsp)

	return reqbody, nil
}

// Encode 服务端打包rspbody到二进制 回给客户端
func (s *ServerCodec) Encode(msg codec.Msg, rspbody []byte) (rspbuf []byte, err error) {
	// 取出回包包头
	rsp, ok := msg.ServerRspHead().(*ResponseProtocol)
	if !ok {
		rsp = &ResponseProtocol{}
	}

	// 更新序列化类型和压缩类型
	rsp.ContentType = uint32(msg.SerializationType())
	rsp.ContentEncoding = uint32(msg.CompressType())

	// 将处理函数handler返回的error转成协议包头里面的错误码字段
	if e := msg.ServerRspErr(); e != nil {
		rsp.ErrorMsg = []byte(e.Msg)
		if e.Type == errs.ErrorTypeFramework {
			rsp.Ret = e.Code
		} else {
			rsp.FuncRet = e.Code
		}
	}

	if len(msg.ServerMetaData()) > 0 {
		if rsp.TransInfo == nil {
			rsp.TransInfo = make(map[string][]byte)
		}
		for k, v := range msg.ServerMetaData() {
			rsp.TransInfo[k] = v
		}
	}

	rsphead, err := proto.Marshal(rsp)
	if err != nil {
		return nil, err
	}

	return s.writeRspBuf(msg, rsphead, rspbody)
}

func (s *ServerCodec) updateMsg(msg codec.Msg, frameHead *FrameHead, req *RequestProtocol, rsp *ResponseProtocol) {
	msg.WithFrameHead(frameHead)
	// 设置具体业务协议请求包头
	msg.WithServerReqHead(req)
	msg.WithServerRspHead(rsp)

	//-----------------以下为trpc框架需要的数据-----------------------------//
	// 设置上游允许的超时时间
	msg.WithRequestTimeout(time.Millisecond * time.Duration(req.GetTimeout()))
	// 设置上游服务名
	msg.WithCallerServiceName(string(req.GetCaller()))
	msg.WithCalleeServiceName(string(req.GetCallee()))
	// 设置当前请求的rpc方法名(命令字)
	msg.WithServerRPCName(string(req.GetFunc()))
	// 设置body的序列化方式
	msg.WithSerializationType(int(req.GetContentType()))
	// 设置body压缩方式
	msg.WithCompressType(int(req.GetContentEncoding()))
	// 设置染色标记
	msg.WithDyeing((req.GetMessageType() & uint32(TrpcMessageType_TRPC_DYEING_MESSAGE)) != 0)
	// 解析tracing MetaData，设置MetaData到msg
	if len(req.TransInfo) > 0 {
		msg.WithServerMetaData(req.GetTransInfo())
		// 染色标记
		if bs, ok := req.TransInfo[DyeingKey]; ok {
			msg.WithDyeingKey(string(bs))
		}
		// 透传环境信息
		if envs, ok := req.TransInfo[EnvTransfer]; ok {
			msg.WithEnvTransfer(string(envs))
		}
	}
}

func (s *ServerCodec) writeRspBuf(msg codec.Msg, rsphead, rspbody []byte) ([]byte, error) {
	totalLen := uint32(frameHeadLen) + uint32(len(rsphead)) + uint32(len(rspbody))
	pbHeadLen := uint16(len(rsphead))

	var err error

	// 开始打包
	buf := bytes.NewBuffer(make([]byte, 0, totalLen))
	if err = binary.Write(buf, binary.BigEndian, stx); err != nil {
		return nil, err
	}
	frameHead, ok := msg.FrameHead().(*FrameHead)
	if !ok {
		return nil, errors.New("framehead type invalid")
	}
	if err = binary.Write(buf, binary.BigEndian, frameHead.FrameType); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, frameHead.FrameStatus); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, totalLen); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, pbHeadLen); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, frameHead.StreamID); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, frameHead.FrameReserved); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, rsphead); err != nil {
		return nil, err
	}
	if err = binary.Write(buf, binary.BigEndian, rspbody); err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

// ClientCodec trpc客户端编解码
type ClientCodec struct {
	RequestID uint32 //全局唯一request id
}

// updateReq 更新请求头
func (c *ClientCodec) updateReqHead(msg codec.Msg, req *RequestProtocol) {
	// 设置调用方 service name
	req.Caller = []byte(msg.CallerServiceName())
	// 设置被调方 service name
	req.Callee = []byte(msg.CalleeServiceName())
	// 设置后端函数rpc方法名，由client stub外层设置
	req.Func = []byte(msg.ClientRPCName())
	// 设置后端序列化方式
	req.ContentType = uint32(msg.SerializationType())
	// 设置后端解压缩方式
	req.ContentEncoding = uint32(msg.CompressType())
	// 设置下游剩余超时时间
	req.Timeout = uint32(msg.RequestTimeout() / time.Millisecond)
	// 设置tracing MetaData
	if len(msg.ClientMetaData()) > 0 {
		if req.TransInfo == nil {
			req.TransInfo = make(map[string][]byte)
		}
		for k, v := range msg.ClientMetaData() {
			req.TransInfo[k] = v
		}
	}
	// 设置染色信息
	if msg.Dyeing() {
		req.MessageType = req.MessageType | uint32(TrpcMessageType_TRPC_DYEING_MESSAGE)
	}
	if len(msg.DyeingKey()) > 0 {
		if req.TransInfo == nil {
			req.TransInfo = make(map[string][]byte)
		}
		req.TransInfo[DyeingKey] = []byte(msg.DyeingKey())
	}
	// 设置透传环境信息
	if len(msg.EnvTransfer()) > 0 {
		if req.TransInfo == nil {
			req.TransInfo = make(map[string][]byte)
		}
		req.TransInfo[EnvTransfer] = []byte(msg.EnvTransfer())
	}
}

// Encode 客户端打包reqbody到二进制数据 发到服务端, client stub会自动clone生成新的msg
func (c *ClientCodec) Encode(msg codec.Msg, reqbody []byte) (reqbuf []byte, err error) {
	req, err := c.getRequestHead(msg)
	if err != nil {
		return nil, err
	}

	// 框架自动生成全局唯一request id
	if req.GetRequestId() == 0 {
		req.RequestId = atomic.AddUint32(&c.RequestID, 1)
	}

	c.updateMsg(msg)

	c.updateReqHead(msg, req)

	reqhead, err := proto.Marshal(req)
	if err != nil {
		return nil, err
	}

	return c.getReqbuf(msg, reqhead, reqbody)
}

func (c *ClientCodec) getRequestHead(msg codec.Msg) (*RequestProtocol, error) {
	// 构造后端请求包头
	if msg.ClientReqHead() != nil {
		// client req head不为空 说明是用户自己创建，直接使用即可
		req, ok := msg.ClientReqHead().(*RequestProtocol)
		if !ok {
			return nil, errors.New("client encode req head type invalid")
		}
		return req, nil
	}

	// client req head为空，需要复制server req head, clone防止并发问题
	req, ok := msg.ServerReqHead().(*RequestProtocol)
	if ok {
		return proto.Clone(req).(*RequestProtocol), nil
	}

	req = &RequestProtocol{
		Version:  uint32(TrpcProtoVersion_TRPC_PROTO_V1),
		CallType: uint32(TrpcCallType_TRPC_UNARY_CALL),
	}
	// 保存新的client req head
	msg.WithClientReqHead(req)

	return req, nil
}

func (c *ClientCodec) getReqbuf(msg codec.Msg, reqhead, reqbody []byte) ([]byte, error) {
	totalLen := uint32(frameHeadLen) + uint32(len(reqhead)) + uint32(len(reqbody))
	pbHeadLen := uint16(len(reqhead))

	// 开始打包
	buf := bytes.NewBuffer(make([]byte, 0, totalLen))
	if err := binary.Write(buf, binary.BigEndian, stx); err != nil {
		return nil, err
	}
	frameHead, ok := msg.FrameHead().(*FrameHead)
	if !ok {
		frameHead = &FrameHead{}
	}
	if err := binary.Write(buf, binary.BigEndian, frameHead.FrameType); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, frameHead.FrameStatus); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, totalLen); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, pbHeadLen); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, frameHead.StreamID); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, frameHead.FrameReserved); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, reqhead); err != nil {
		return nil, err
	}
	if err := binary.Write(buf, binary.BigEndian, reqbody); err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (c *ClientCodec) updateMsg(msg codec.Msg) {
	// 如果调用方为空 则取进程名, client小工具，没有caller
	if msg.CallerServiceName() == "" {
		msg.WithCallerServiceName(fmt.Sprintf("trpc.app.%s.service", path.Base(os.Args[0])))
	}
}

// Decode 客户端收到服务端二进制回包数据解包到rspbody
func (c *ClientCodec) Decode(msg codec.Msg, rspbuf []byte) (rspbody []byte, err error) {
	if len(rspbuf) < int(frameHeadLen) {
		return nil, errors.New("client decode rsp buf len invalid")
	}

	// 构造后端响应包头
	var rsp *ResponseProtocol
	if msg.ClientRspHead() != nil {
		// client rsp head不为空 说明是用户故意创建，希望底层回传后端响应包头
		response, ok := msg.ClientRspHead().(*ResponseProtocol)
		if !ok {
			return nil, errors.New("client decode rsp head type invalid")
		}
		rsp = response
	} else {
		// client rsp head为空 说明用户不关心后端响应包头
		rsp = &ResponseProtocol{}
		// 保存新的client rsp head
		msg.WithClientRspHead(rsp)
	}

	// 解响应包头
	pbHeadLen := binary.BigEndian.Uint16(rspbuf[8:10])
	if pbHeadLen == 0 {
		return nil, errors.New("client decode pb head len empty")
	}
	if int(pbHeadLen) > len(rspbuf)-int(frameHeadLen) {
		return nil, errors.New("client decode pb head len invalid")
	}
	if err := proto.Unmarshal(rspbuf[frameHeadLen:frameHeadLen+pbHeadLen], rsp); err != nil {
		return nil, err
	}

	frameHead := &FrameHead{
		FrameType:     rspbuf[2],
		FrameStatus:   rspbuf[3],
		StreamID:      binary.BigEndian.Uint16(rspbuf[10:12]),
		FrameReserved: binary.BigEndian.Uint32(rspbuf[12:16]),
	}
	msg.WithFrameHead(frameHead)

	msg.WithCompressType(int(rsp.GetContentEncoding()))
	msg.WithSerializationType(int(rsp.GetContentType()))

	if len(rsp.TransInfo) > 0 { // 重新设置透传字段，下游有可能返回新的透传信息
		md := msg.ClientMetaData()
		if len(md) == 0 {
			md = codec.MetaData{}
		}
		for k, v := range rsp.TransInfo {
			md[k] = v
		}
		msg.WithClientMetaData(md)
	}

	// 将业务协议包头错误码转化成err返回给调用用户
	if rsp.GetRet() != 0 {
		e := &errs.Error{
			Type: errs.ErrorTypeCalleeFramework,
			Code: rsp.GetRet(),
			Desc: "trpc",
			Msg:  string(rsp.GetErrorMsg()),
		}
		msg.WithClientRspErr(e)
	} else if rsp.GetFuncRet() != 0 {
		msg.WithClientRspErr(errs.New(int(rsp.GetFuncRet()), string(rsp.GetErrorMsg())))
	}

	// 解响应包体
	rspbody = rspbuf[frameHeadLen+pbHeadLen:]

	return rspbody, nil
}
