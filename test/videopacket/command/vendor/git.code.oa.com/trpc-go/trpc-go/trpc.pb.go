// Code generated by protoc-gen-go. DO NOT EDIT.
// source: trpc.proto

package trpc

import (
	fmt "fmt"
	proto "github.com/golang/protobuf/proto"
	math "math"
)

// Reference imports to suppress errors if they are not otherwise used.
var _ = proto.Marshal
var _ = fmt.Errorf
var _ = math.Inf

// This is a compile-time assertion to ensure that this generated file
// is compatible with the proto package it is being compiled against.
// A compilation error at this line likely means your copy of the
// proto package needs to be updated.
const _ = proto.ProtoPackageIsVersion3 // please upgrade the proto package

// 框架协议头里的魔数
type TrpcMagic int32

const (
	// trpc不用这个值，为了提供给pb工具生成代码
	TrpcMagic_TRPC_DEFAULT_NONE TrpcMagic = 0
	// trpc协议默认使用这个魔数
	TrpcMagic_TRPC_MAGIC_VALUE TrpcMagic = 2352
)

var TrpcMagic_name = map[int32]string{
	0:    "TRPC_DEFAULT_NONE",
	2352: "TRPC_MAGIC_VALUE",
}

var TrpcMagic_value = map[string]int32{
	"TRPC_DEFAULT_NONE": 0,
	"TRPC_MAGIC_VALUE":  2352,
}

func (x TrpcMagic) String() string {
	return proto.EnumName(TrpcMagic_name, int32(x))
}

func (TrpcMagic) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{0}
}

// trpc协议的二进制数据帧类型
// 目前支持两种类型的二进制数据帧：
// 1. 一应一答模式的二进制数据帧类型
// 2. 流式模式的二进制数据帧类型
type TrpcDataFrameType int32

const (
	// trpc一应一答模式的二进制数据帧类型
	TrpcDataFrameType_TRPC_UNARY_FRAME TrpcDataFrameType = 0
	// trpc流式模式的二进制数据帧类型
	TrpcDataFrameType_TRPC_STREAM_FRAME TrpcDataFrameType = 1
)

var TrpcDataFrameType_name = map[int32]string{
	0: "TRPC_UNARY_FRAME",
	1: "TRPC_STREAM_FRAME",
}

var TrpcDataFrameType_value = map[string]int32{
	"TRPC_UNARY_FRAME":  0,
	"TRPC_STREAM_FRAME": 1,
}

func (x TrpcDataFrameType) String() string {
	return proto.EnumName(TrpcDataFrameType_name, int32(x))
}

func (TrpcDataFrameType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{1}
}

// trpc协议的二进制数据帧的状态
// 目前支持流式模式的二进制数据帧结束状态
type TrpcDataFrameState int32

const (
	// 不包括任何状态
	TrpcDataFrameState_TRPC_NO_STATE TrpcDataFrameState = 0
	// trpc流式模式下的结束状态
	TrpcDataFrameState_TRPC_STREAM_FINISH TrpcDataFrameState = 1
)

var TrpcDataFrameState_name = map[int32]string{
	0: "TRPC_NO_STATE",
	1: "TRPC_STREAM_FINISH",
}

var TrpcDataFrameState_value = map[string]int32{
	"TRPC_NO_STATE":      0,
	"TRPC_STREAM_FINISH": 1,
}

func (x TrpcDataFrameState) String() string {
	return proto.EnumName(TrpcDataFrameState_name, int32(x))
}

func (TrpcDataFrameState) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{2}
}

// trpc协议版本
type TrpcProtoVersion int32

const (
	// 默认版本
	TrpcProtoVersion_TRPC_PROTO_V1 TrpcProtoVersion = 0
)

var TrpcProtoVersion_name = map[int32]string{
	0: "TRPC_PROTO_V1",
}

var TrpcProtoVersion_value = map[string]int32{
	"TRPC_PROTO_V1": 0,
}

func (x TrpcProtoVersion) String() string {
	return proto.EnumName(TrpcProtoVersion_name, int32(x))
}

func (TrpcProtoVersion) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{3}
}

// trpc协议中的调用类型
type TrpcCallType int32

const (
	// 一应一答调用，包括同步、异步
	TrpcCallType_TRPC_UNARY_CALL TrpcCallType = 0
	// 单向调用
	TrpcCallType_TRPC_ONEWAY_CALL TrpcCallType = 1
	// 客户端流式请求调用
	TrpcCallType_TRPC_CLIENT_STREAM_CALL TrpcCallType = 2
	// 服务端流式回应
	TrpcCallType_TRPC_SERVER_STREAM_CALL TrpcCallType = 3
	// 客户端和服务端流式请求和回应
	TrpcCallType_TRPC_BIDI_STREAM_CALL TrpcCallType = 4
)

var TrpcCallType_name = map[int32]string{
	0: "TRPC_UNARY_CALL",
	1: "TRPC_ONEWAY_CALL",
	2: "TRPC_CLIENT_STREAM_CALL",
	3: "TRPC_SERVER_STREAM_CALL",
	4: "TRPC_BIDI_STREAM_CALL",
}

var TrpcCallType_value = map[string]int32{
	"TRPC_UNARY_CALL":         0,
	"TRPC_ONEWAY_CALL":        1,
	"TRPC_CLIENT_STREAM_CALL": 2,
	"TRPC_SERVER_STREAM_CALL": 3,
	"TRPC_BIDI_STREAM_CALL":   4,
}

func (x TrpcCallType) String() string {
	return proto.EnumName(TrpcCallType_name, int32(x))
}

func (TrpcCallType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{4}
}

// trpc协议中的消息透传支持的类型
type TrpcMessageType int32

const (
	// trpc 不用这个值，为了提供给 pb 工具生成代码
	TrpcMessageType_TRPC_DEFAULT TrpcMessageType = 0
	// 染色
	TrpcMessageType_TRPC_DYEING_MESSAGE TrpcMessageType = 1
	// 调用链
	TrpcMessageType_TRPC_TRACE_MESSAGE TrpcMessageType = 2
	// 多环境
	TrpcMessageType_TRPC_MULTI_ENV_MESSAGE TrpcMessageType = 4
	// 灰度
	TrpcMessageType_TRPC_GRID_MESSAGE TrpcMessageType = 8
	// set名
	TrpcMessageType_TRPC_SETNAME_MESSAGE TrpcMessageType = 16
)

var TrpcMessageType_name = map[int32]string{
	0:  "TRPC_DEFAULT",
	1:  "TRPC_DYEING_MESSAGE",
	2:  "TRPC_TRACE_MESSAGE",
	4:  "TRPC_MULTI_ENV_MESSAGE",
	8:  "TRPC_GRID_MESSAGE",
	16: "TRPC_SETNAME_MESSAGE",
}

var TrpcMessageType_value = map[string]int32{
	"TRPC_DEFAULT":           0,
	"TRPC_DYEING_MESSAGE":    1,
	"TRPC_TRACE_MESSAGE":     2,
	"TRPC_MULTI_ENV_MESSAGE": 4,
	"TRPC_GRID_MESSAGE":      8,
	"TRPC_SETNAME_MESSAGE":   16,
}

func (x TrpcMessageType) String() string {
	return proto.EnumName(TrpcMessageType_name, int32(x))
}

func (TrpcMessageType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{5}
}

// trpc协议中 data 内容的编码类型
// 默认使用pb
// 目前约定 0-127 范围的取值为框架规范的序列化方式,框架使用
type TrpcContentEncodeType int32

const (
	// pb
	TrpcContentEncodeType_TRPC_PROTO_ENCODE TrpcContentEncodeType = 0
	// jce
	TrpcContentEncodeType_TRPC_JCE_ENCODE TrpcContentEncodeType = 1
	// json
	TrpcContentEncodeType_TRPC_JSON_ENCODE TrpcContentEncodeType = 2
	// flatbuffer
	TrpcContentEncodeType_TRPC_FLATBUFFER_ENCODE TrpcContentEncodeType = 3
	// 不序列化
	TrpcContentEncodeType_TRPC_NOOP_ENCODE TrpcContentEncodeType = 4
)

var TrpcContentEncodeType_name = map[int32]string{
	0: "TRPC_PROTO_ENCODE",
	1: "TRPC_JCE_ENCODE",
	2: "TRPC_JSON_ENCODE",
	3: "TRPC_FLATBUFFER_ENCODE",
	4: "TRPC_NOOP_ENCODE",
}

var TrpcContentEncodeType_value = map[string]int32{
	"TRPC_PROTO_ENCODE":      0,
	"TRPC_JCE_ENCODE":        1,
	"TRPC_JSON_ENCODE":       2,
	"TRPC_FLATBUFFER_ENCODE": 3,
	"TRPC_NOOP_ENCODE":       4,
}

func (x TrpcContentEncodeType) String() string {
	return proto.EnumName(TrpcContentEncodeType_name, int32(x))
}

func (TrpcContentEncodeType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{6}
}

// trpc协议中 data 内容的压缩类型
// 默认使用不压缩
type TrpcCompressType int32

const (
	// 默认不使用压缩
	TrpcCompressType_TRPC_DEFAULT_COMPRESS TrpcCompressType = 0
	// 使用gzip
	TrpcCompressType_TRPC_GZIP_COMPRESS TrpcCompressType = 1
	// 使用snappy
	TrpcCompressType_TRPC_SNAPPY_COMPRESS TrpcCompressType = 2
)

var TrpcCompressType_name = map[int32]string{
	0: "TRPC_DEFAULT_COMPRESS",
	1: "TRPC_GZIP_COMPRESS",
	2: "TRPC_SNAPPY_COMPRESS",
}

var TrpcCompressType_value = map[string]int32{
	"TRPC_DEFAULT_COMPRESS": 0,
	"TRPC_GZIP_COMPRESS":    1,
	"TRPC_SNAPPY_COMPRESS":  2,
}

func (x TrpcCompressType) String() string {
	return proto.EnumName(TrpcCompressType_name, int32(x))
}

func (TrpcCompressType) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{7}
}

// 框架层接口调用的返回码定义
type TrpcRetCode int32

const (
	// 调用成功
	TrpcRetCode_TRPC_INVOKE_SUCCESS TrpcRetCode = 0
	// 协议错误码
	// 服务端解码错误
	TrpcRetCode_TRPC_SERVER_DECODE_ERR TrpcRetCode = 1
	// 服务端编码错误
	TrpcRetCode_TRPC_SERVER_ENCODE_ERR TrpcRetCode = 2
	// service或者func路由错误码
	// 服务端没有调用相应的service实现
	TrpcRetCode_TRPC_SERVER_NOSERVICE_ERR TrpcRetCode = 11
	// 服务端没有调用相应的接口实现
	TrpcRetCode_TRPC_SERVER_NOFUNC_ERR TrpcRetCode = 12
	// 队列超时或过载错误码
	// 请求在服务端超时
	TrpcRetCode_TRPC_SERVER_TIMEOUT_ERR TrpcRetCode = 21
	// 请求在服务端过载
	TrpcRetCode_TRPC_SERVER_OVERLOAD_ERR TrpcRetCode = 22
	// 服务端系统错误
	TrpcRetCode_TRPC_SERVER_SYSTEM_ERR TrpcRetCode = 31
	// 服务端鉴权失败错误
	TrpcRetCode_TRPC_SERVER_AUTH_ERR TrpcRetCode = 41
	// 服务端请求参数自动校验失败错误
	TrpcRetCode_TRPC_SERVER_VALIDATE_ERR TrpcRetCode = 51
	// 超时错误码
	// 请求在客户端调用超时
	TrpcRetCode_TRPC_CLIENT_INVOKE_TIMEOUT_ERR TrpcRetCode = 101
	// 网络相关错误码
	// 客户端连接错误
	TrpcRetCode_TRPC_CLIENT_CONNECT_ERR TrpcRetCode = 111
	// 协议相关错误码
	// 客户端编码错误
	TrpcRetCode_TRPC_CLIENT_ENCODE_ERR TrpcRetCode = 121
	// 客户端解码错误
	TrpcRetCode_TRPC_CLIENT_DECODE_ERR TrpcRetCode = 122
	// 路由相关错误码
	// 客户端选ip路由错误
	TrpcRetCode_TRPC_CLIENT_ROUTER_ERR TrpcRetCode = 131
	// 客户端网络错误
	TrpcRetCode_TRPC_CLIENT_NETWORK_ERR TrpcRetCode = 141
	// 客户端响应参数自动校验失败错误
	TrpcRetCode_TRPC_CLIENT_VALIDATE_ERR TrpcRetCode = 151
	// 上游主动断开连接，提前取消请求错误
	TrpcRetCode_TRPC_CLIENT_CANCELED_ERR TrpcRetCode = 161
	// 未明确的错误
	TrpcRetCode_TRPC_INVOKE_UNKNOWN_ERR TrpcRetCode = 999
)

var TrpcRetCode_name = map[int32]string{
	0:   "TRPC_INVOKE_SUCCESS",
	1:   "TRPC_SERVER_DECODE_ERR",
	2:   "TRPC_SERVER_ENCODE_ERR",
	11:  "TRPC_SERVER_NOSERVICE_ERR",
	12:  "TRPC_SERVER_NOFUNC_ERR",
	21:  "TRPC_SERVER_TIMEOUT_ERR",
	22:  "TRPC_SERVER_OVERLOAD_ERR",
	31:  "TRPC_SERVER_SYSTEM_ERR",
	41:  "TRPC_SERVER_AUTH_ERR",
	51:  "TRPC_SERVER_VALIDATE_ERR",
	101: "TRPC_CLIENT_INVOKE_TIMEOUT_ERR",
	111: "TRPC_CLIENT_CONNECT_ERR",
	121: "TRPC_CLIENT_ENCODE_ERR",
	122: "TRPC_CLIENT_DECODE_ERR",
	131: "TRPC_CLIENT_ROUTER_ERR",
	141: "TRPC_CLIENT_NETWORK_ERR",
	151: "TRPC_CLIENT_VALIDATE_ERR",
	161: "TRPC_CLIENT_CANCELED_ERR",
	999: "TRPC_INVOKE_UNKNOWN_ERR",
}

var TrpcRetCode_value = map[string]int32{
	"TRPC_INVOKE_SUCCESS":            0,
	"TRPC_SERVER_DECODE_ERR":         1,
	"TRPC_SERVER_ENCODE_ERR":         2,
	"TRPC_SERVER_NOSERVICE_ERR":      11,
	"TRPC_SERVER_NOFUNC_ERR":         12,
	"TRPC_SERVER_TIMEOUT_ERR":        21,
	"TRPC_SERVER_OVERLOAD_ERR":       22,
	"TRPC_SERVER_SYSTEM_ERR":         31,
	"TRPC_SERVER_AUTH_ERR":           41,
	"TRPC_SERVER_VALIDATE_ERR":       51,
	"TRPC_CLIENT_INVOKE_TIMEOUT_ERR": 101,
	"TRPC_CLIENT_CONNECT_ERR":        111,
	"TRPC_CLIENT_ENCODE_ERR":         121,
	"TRPC_CLIENT_DECODE_ERR":         122,
	"TRPC_CLIENT_ROUTER_ERR":         131,
	"TRPC_CLIENT_NETWORK_ERR":        141,
	"TRPC_CLIENT_VALIDATE_ERR":       151,
	"TRPC_CLIENT_CANCELED_ERR":       161,
	"TRPC_INVOKE_UNKNOWN_ERR":        999,
}

func (x TrpcRetCode) String() string {
	return proto.EnumName(TrpcRetCode_name, int32(x))
}

func (TrpcRetCode) EnumDescriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{8}
}

// 请求协议头
type RequestProtocol struct {
	// 协议版本
	// 具体值与TrpcProtoVersion对应
	Version uint32 `protobuf:"varint,1,opt,name=version,proto3" json:"version,omitempty"`
	// 请求的调用类型
	// 比如: 普通调用，单向调用，流式调用
	// 具体值与TrpcCallType对应
	CallType uint32 `protobuf:"varint,2,opt,name=call_type,json=callType,proto3" json:"call_type,omitempty"`
	// 请求唯一id
	RequestId uint32 `protobuf:"varint,3,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	// 请求的超时时间，单位ms
	Timeout uint32 `protobuf:"varint,4,opt,name=timeout,proto3" json:"timeout,omitempty"`
	// 主调服务的名称
	// trpc协议下的规范格式: trpc.应用名.服务名.pb的service名, 4段
	Caller []byte `protobuf:"bytes,5,opt,name=caller,proto3" json:"caller,omitempty"`
	// 被调服务的路由名称
	// trpc协议下的规范格式，trpc.应用名.服务名.pb的service名[.接口名]
	// 前4段是必须有，接口可选。
	Callee []byte `protobuf:"bytes,6,opt,name=callee,proto3" json:"callee,omitempty"`
	// 调用服务的接口名
	// 规范格式: /package.Service名称/接口名
	Func []byte `protobuf:"bytes,7,opt,name=func,proto3" json:"func,omitempty"`
	// 框架信息透传的消息类型
	// 比如调用链、染色key、灰度、鉴权、多环境、set名称等的标识
	// 具体值与TrpcMessageType对应
	MessageType uint32 `protobuf:"varint,8,opt,name=message_type,json=messageType,proto3" json:"message_type,omitempty"`
	// 框架透传的信息key-value对，目前分两部分
	// 1是框架层要透传的信息，key的名字要以trpc-开头
	// 2是业务层要透传的信息，业务可以自行设置
	TransInfo map[string][]byte `protobuf:"bytes,9,rep,name=trans_info,json=transInfo,proto3" json:"trans_info,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	// 请求数据的序列化类型
	// 比如: proto/jce/json, 默认proto
	// 具体值与TrpcContentEncodeType对应
	ContentType uint32 `protobuf:"varint,10,opt,name=content_type,json=contentType,proto3" json:"content_type,omitempty"`
	// 请求数据使用的压缩方式
	// 比如: gzip/snappy/..., 默认不使用
	// 具体值与TrpcCompressType对应
	ContentEncoding      uint32   `protobuf:"varint,11,opt,name=content_encoding,json=contentEncoding,proto3" json:"content_encoding,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *RequestProtocol) Reset()         { *m = RequestProtocol{} }
func (m *RequestProtocol) String() string { return proto.CompactTextString(m) }
func (*RequestProtocol) ProtoMessage()    {}
func (*RequestProtocol) Descriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{0}
}

func (m *RequestProtocol) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_RequestProtocol.Unmarshal(m, b)
}
func (m *RequestProtocol) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_RequestProtocol.Marshal(b, m, deterministic)
}
func (m *RequestProtocol) XXX_Merge(src proto.Message) {
	xxx_messageInfo_RequestProtocol.Merge(m, src)
}
func (m *RequestProtocol) XXX_Size() int {
	return xxx_messageInfo_RequestProtocol.Size(m)
}
func (m *RequestProtocol) XXX_DiscardUnknown() {
	xxx_messageInfo_RequestProtocol.DiscardUnknown(m)
}

var xxx_messageInfo_RequestProtocol proto.InternalMessageInfo

func (m *RequestProtocol) GetVersion() uint32 {
	if m != nil {
		return m.Version
	}
	return 0
}

func (m *RequestProtocol) GetCallType() uint32 {
	if m != nil {
		return m.CallType
	}
	return 0
}

func (m *RequestProtocol) GetRequestId() uint32 {
	if m != nil {
		return m.RequestId
	}
	return 0
}

func (m *RequestProtocol) GetTimeout() uint32 {
	if m != nil {
		return m.Timeout
	}
	return 0
}

func (m *RequestProtocol) GetCaller() []byte {
	if m != nil {
		return m.Caller
	}
	return nil
}

func (m *RequestProtocol) GetCallee() []byte {
	if m != nil {
		return m.Callee
	}
	return nil
}

func (m *RequestProtocol) GetFunc() []byte {
	if m != nil {
		return m.Func
	}
	return nil
}

func (m *RequestProtocol) GetMessageType() uint32 {
	if m != nil {
		return m.MessageType
	}
	return 0
}

func (m *RequestProtocol) GetTransInfo() map[string][]byte {
	if m != nil {
		return m.TransInfo
	}
	return nil
}

func (m *RequestProtocol) GetContentType() uint32 {
	if m != nil {
		return m.ContentType
	}
	return 0
}

func (m *RequestProtocol) GetContentEncoding() uint32 {
	if m != nil {
		return m.ContentEncoding
	}
	return 0
}

// 响应协议头
type ResponseProtocol struct {
	// 协议版本
	// 具体值与TrpcProtoVersion对应
	Version uint32 `protobuf:"varint,1,opt,name=version,proto3" json:"version,omitempty"`
	// 请求的调用类型
	// 比如: 普通调用，单向调用，流式调用
	// 具体值与TrpcCallType对应
	CallType uint32 `protobuf:"varint,2,opt,name=call_type,json=callType,proto3" json:"call_type,omitempty"`
	// 请求唯一id
	RequestId uint32 `protobuf:"varint,3,opt,name=request_id,json=requestId,proto3" json:"request_id,omitempty"`
	// 请求在框架层的错误返回码
	// 具体值与TrpcRetCode对应
	Ret int32 `protobuf:"varint,4,opt,name=ret,proto3" json:"ret,omitempty"`
	// 接口的错误返回码
	// 建议业务在使用时，标识成功和失败，0代表成功，其它代表失败
	FuncRet int32 `protobuf:"varint,5,opt,name=func_ret,json=funcRet,proto3" json:"func_ret,omitempty"`
	// 调用结果信息描述
	// 失败的时候用
	ErrorMsg []byte `protobuf:"bytes,6,opt,name=error_msg,json=errorMsg,proto3" json:"error_msg,omitempty"`
	// 框架信息透传的消息类型
	// 比如调用链、染色key、灰度、鉴权、多环境、set名称等的标识
	// 具体值与TrpcMessageType对应
	MessageType uint32 `protobuf:"varint,7,opt,name=message_type,json=messageType,proto3" json:"message_type,omitempty"`
	// 框架透传回来的信息key-value对，
	// 目前分两部分
	// 1是框架层透传回来的信息，key的名字要以trpc-开头
	// 2是业务层透传回来的信息，业务可以自行设置
	TransInfo map[string][]byte `protobuf:"bytes,8,rep,name=trans_info,json=transInfo,proto3" json:"trans_info,omitempty" protobuf_key:"bytes,1,opt,name=key,proto3" protobuf_val:"bytes,2,opt,name=value,proto3"`
	// 响应数据的编码类型
	// 比如: proto/jce/json, 默认proto
	// 具体值与TrpcContentEncodeType对应
	ContentType uint32 `protobuf:"varint,9,opt,name=content_type,json=contentType,proto3" json:"content_type,omitempty"`
	// 响应数据使用的压缩方式
	// 比如: gzip/snappy/..., 默认不使用
	// 具体值与TrpcCompressType对应
	ContentEncoding      uint32   `protobuf:"varint,10,opt,name=content_encoding,json=contentEncoding,proto3" json:"content_encoding,omitempty"`
	XXX_NoUnkeyedLiteral struct{} `json:"-"`
	XXX_unrecognized     []byte   `json:"-"`
	XXX_sizecache        int32    `json:"-"`
}

func (m *ResponseProtocol) Reset()         { *m = ResponseProtocol{} }
func (m *ResponseProtocol) String() string { return proto.CompactTextString(m) }
func (*ResponseProtocol) ProtoMessage()    {}
func (*ResponseProtocol) Descriptor() ([]byte, []int) {
	return fileDescriptor_b55610ebff55e10c, []int{1}
}

func (m *ResponseProtocol) XXX_Unmarshal(b []byte) error {
	return xxx_messageInfo_ResponseProtocol.Unmarshal(m, b)
}
func (m *ResponseProtocol) XXX_Marshal(b []byte, deterministic bool) ([]byte, error) {
	return xxx_messageInfo_ResponseProtocol.Marshal(b, m, deterministic)
}
func (m *ResponseProtocol) XXX_Merge(src proto.Message) {
	xxx_messageInfo_ResponseProtocol.Merge(m, src)
}
func (m *ResponseProtocol) XXX_Size() int {
	return xxx_messageInfo_ResponseProtocol.Size(m)
}
func (m *ResponseProtocol) XXX_DiscardUnknown() {
	xxx_messageInfo_ResponseProtocol.DiscardUnknown(m)
}

var xxx_messageInfo_ResponseProtocol proto.InternalMessageInfo

func (m *ResponseProtocol) GetVersion() uint32 {
	if m != nil {
		return m.Version
	}
	return 0
}

func (m *ResponseProtocol) GetCallType() uint32 {
	if m != nil {
		return m.CallType
	}
	return 0
}

func (m *ResponseProtocol) GetRequestId() uint32 {
	if m != nil {
		return m.RequestId
	}
	return 0
}

func (m *ResponseProtocol) GetRet() int32 {
	if m != nil {
		return m.Ret
	}
	return 0
}

func (m *ResponseProtocol) GetFuncRet() int32 {
	if m != nil {
		return m.FuncRet
	}
	return 0
}

func (m *ResponseProtocol) GetErrorMsg() []byte {
	if m != nil {
		return m.ErrorMsg
	}
	return nil
}

func (m *ResponseProtocol) GetMessageType() uint32 {
	if m != nil {
		return m.MessageType
	}
	return 0
}

func (m *ResponseProtocol) GetTransInfo() map[string][]byte {
	if m != nil {
		return m.TransInfo
	}
	return nil
}

func (m *ResponseProtocol) GetContentType() uint32 {
	if m != nil {
		return m.ContentType
	}
	return 0
}

func (m *ResponseProtocol) GetContentEncoding() uint32 {
	if m != nil {
		return m.ContentEncoding
	}
	return 0
}

func init() {
	proto.RegisterEnum("trpc.TrpcMagic", TrpcMagic_name, TrpcMagic_value)
	proto.RegisterEnum("trpc.TrpcDataFrameType", TrpcDataFrameType_name, TrpcDataFrameType_value)
	proto.RegisterEnum("trpc.TrpcDataFrameState", TrpcDataFrameState_name, TrpcDataFrameState_value)
	proto.RegisterEnum("trpc.TrpcProtoVersion", TrpcProtoVersion_name, TrpcProtoVersion_value)
	proto.RegisterEnum("trpc.TrpcCallType", TrpcCallType_name, TrpcCallType_value)
	proto.RegisterEnum("trpc.TrpcMessageType", TrpcMessageType_name, TrpcMessageType_value)
	proto.RegisterEnum("trpc.TrpcContentEncodeType", TrpcContentEncodeType_name, TrpcContentEncodeType_value)
	proto.RegisterEnum("trpc.TrpcCompressType", TrpcCompressType_name, TrpcCompressType_value)
	proto.RegisterEnum("trpc.TrpcRetCode", TrpcRetCode_name, TrpcRetCode_value)
	proto.RegisterType((*RequestProtocol)(nil), "trpc.RequestProtocol")
	proto.RegisterMapType((map[string][]byte)(nil), "trpc.RequestProtocol.TransInfoEntry")
	proto.RegisterType((*ResponseProtocol)(nil), "trpc.ResponseProtocol")
	proto.RegisterMapType((map[string][]byte)(nil), "trpc.ResponseProtocol.TransInfoEntry")
}

func init() {
	proto.RegisterFile("trpc.proto", fileDescriptor_b55610ebff55e10c)
}

var fileDescriptor_b55610ebff55e10c = []byte{
	// 1033 bytes of a gzipped FileDescriptorProto
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0xb4, 0x56, 0x41, 0x6f, 0xe3, 0x44,
	0x18, 0xad, 0x93, 0xb4, 0x4d, 0xbe, 0x66, 0xa9, 0x3b, 0xdb, 0x76, 0xdd, 0x6d, 0x0b, 0xa5, 0x62,
	0x51, 0x37, 0x12, 0x41, 0xb0, 0x17, 0x40, 0x48, 0xe0, 0x3a, 0x93, 0xae, 0xb7, 0xc9, 0x38, 0x1a,
	0x4f, 0x52, 0x75, 0x2f, 0x96, 0x71, 0xa6, 0x51, 0x44, 0x63, 0x07, 0xdb, 0x5d, 0xa9, 0x5c, 0x39,
	0x22, 0xb8, 0x72, 0xde, 0x13, 0x57, 0x8e, 0xf0, 0x8b, 0xf8, 0x1b, 0x68, 0x66, 0x6c, 0xc7, 0x49,
	0x2f, 0xec, 0x61, 0x4f, 0x9d, 0x79, 0x6f, 0xe6, 0x7d, 0xdf, 0xbc, 0xf7, 0x59, 0x0d, 0x40, 0x1a,
	0xcf, 0x83, 0xf6, 0x3c, 0x8e, 0xd2, 0x08, 0xd5, 0xc4, 0xfa, 0xf4, 0x9f, 0x2a, 0x6c, 0x53, 0xfe,
	0xd3, 0x1d, 0x4f, 0xd2, 0x81, 0x80, 0x83, 0xe8, 0x16, 0x19, 0xb0, 0xf9, 0x86, 0xc7, 0xc9, 0x34,
	0x0a, 0x0d, 0xed, 0x44, 0x3b, 0x7b, 0x44, 0xf3, 0x2d, 0x3a, 0x84, 0x46, 0xe0, 0xdf, 0xde, 0x7a,
	0xe9, 0xfd, 0x9c, 0x1b, 0x15, 0xc9, 0xd5, 0x05, 0xc0, 0xee, 0xe7, 0x1c, 0x1d, 0x03, 0xc4, 0x4a,
	0xc9, 0x9b, 0x8e, 0x8d, 0xaa, 0x64, 0x1b, 0x19, 0x62, 0x8f, 0x85, 0x6a, 0x3a, 0x9d, 0xf1, 0xe8,
	0x2e, 0x35, 0x6a, 0x4a, 0x35, 0xdb, 0xa2, 0x7d, 0xd8, 0x10, 0x22, 0x3c, 0x36, 0xd6, 0x4f, 0xb4,
	0xb3, 0x26, 0xcd, 0x76, 0x05, 0xce, 0x8d, 0x8d, 0x12, 0xce, 0x11, 0x82, 0xda, 0xcd, 0x5d, 0x18,
	0x18, 0x9b, 0x12, 0x95, 0x6b, 0xf4, 0x31, 0x34, 0x67, 0x3c, 0x49, 0xfc, 0x09, 0x57, 0xcd, 0xd5,
	0x65, 0x89, 0xad, 0x0c, 0x93, 0xfd, 0x59, 0xe2, 0xf9, 0x7e, 0x98, 0x78, 0xd3, 0xf0, 0x26, 0x32,
	0x1a, 0x27, 0xd5, 0xb3, 0xad, 0x2f, 0x3f, 0x69, 0x4b, 0x47, 0x56, 0x1c, 0x68, 0x33, 0x71, 0xce,
	0x0e, 0x6f, 0x22, 0x1c, 0xa6, 0xf1, 0x3d, 0x6d, 0xa4, 0xf9, 0x5e, 0xd4, 0x09, 0xa2, 0x30, 0xe5,
	0x61, 0xaa, 0xea, 0x80, 0xaa, 0x93, 0x61, 0xb2, 0xce, 0x73, 0xd0, 0xf3, 0x23, 0x3c, 0x0c, 0xa2,
	0xf1, 0x34, 0x9c, 0x18, 0x5b, 0xf2, 0xd8, 0x76, 0x86, 0xe3, 0x0c, 0x7e, 0xfa, 0x2d, 0x7c, 0xb0,
	0x5c, 0x0a, 0xe9, 0x50, 0xfd, 0x91, 0xdf, 0x4b, 0xdf, 0x1b, 0x54, 0x2c, 0xd1, 0x2e, 0xac, 0xbf,
	0xf1, 0x6f, 0xef, 0x94, 0xdf, 0x4d, 0xaa, 0x36, 0xdf, 0x54, 0xbe, 0xd2, 0x4e, 0xff, 0xac, 0x82,
	0x4e, 0x79, 0x32, 0x8f, 0xc2, 0x84, 0xbf, 0xe7, 0xf0, 0x74, 0xa8, 0xc6, 0x5c, 0x05, 0xb7, 0x4e,
	0xc5, 0x12, 0x1d, 0x40, 0x5d, 0x18, 0xef, 0x09, 0x78, 0x5d, 0xc2, 0x9b, 0x62, 0x4f, 0x79, 0x2a,
	0x0a, 0xf1, 0x38, 0x8e, 0x62, 0x6f, 0x96, 0x4c, 0xb2, 0xe8, 0xea, 0x12, 0xe8, 0x27, 0x93, 0x07,
	0x41, 0x6d, 0x3e, 0x0c, 0xaa, 0xb3, 0x14, 0x54, 0x5d, 0x06, 0xf5, 0x2c, 0x0f, 0x6a, 0xf9, 0xb9,
	0xef, 0x90, 0x54, 0xe3, 0xff, 0x25, 0x05, 0xef, 0x21, 0xa9, 0xd6, 0xd7, 0xd0, 0x60, 0xf1, 0x3c,
	0xe8, 0xfb, 0x93, 0x69, 0x80, 0xf6, 0x60, 0x87, 0xd1, 0x81, 0xe5, 0x75, 0x70, 0xd7, 0x1c, 0xf6,
	0x98, 0x47, 0x1c, 0x82, 0xf5, 0x35, 0xb4, 0x07, 0xba, 0x84, 0xfb, 0xe6, 0x85, 0x6d, 0x79, 0x23,
	0xb3, 0x37, 0xc4, 0xfa, 0x5f, 0xa8, 0xf5, 0x3d, 0xec, 0x88, 0xab, 0x1d, 0x3f, 0xf5, 0xbb, 0xb1,
	0x3f, 0x53, 0x0e, 0xed, 0x66, 0x67, 0x87, 0xc4, 0xa4, 0xd7, 0x5e, 0x97, 0x9a, 0x7d, 0xa5, 0xa0,
	0x84, 0x5d, 0x46, 0xb1, 0xd9, 0xcf, 0x60, 0xad, 0xf5, 0x1d, 0xa0, 0x25, 0x05, 0x37, 0xf5, 0x53,
	0x8e, 0x76, 0xe0, 0x91, 0x3c, 0x4c, 0x1c, 0xcf, 0x65, 0x26, 0x13, 0xf7, 0xf7, 0x01, 0x2d, 0xdd,
	0xb7, 0x89, 0xed, 0xbe, 0xd4, 0xb5, 0xd6, 0x33, 0xd0, 0x85, 0x80, 0xf4, 0x7c, 0x94, 0x0d, 0x53,
	0x7e, 0x7d, 0x40, 0x1d, 0xe6, 0x78, 0xa3, 0x2f, 0xf4, 0xb5, 0xd6, 0xef, 0x1a, 0x34, 0xc5, 0x39,
	0x2b, 0x9f, 0xa9, 0xc7, 0xb0, 0x5d, 0xea, 0xd2, 0x32, 0x7b, 0x3d, 0x7d, 0xad, 0x68, 0xdd, 0x21,
	0xf8, 0xca, 0xcc, 0x50, 0x0d, 0x1d, 0xc2, 0x13, 0x89, 0x5a, 0x3d, 0x1b, 0x13, 0x96, 0x77, 0x20,
	0xc9, 0x4a, 0x41, 0xba, 0x98, 0x8e, 0x30, 0x5d, 0x22, 0xab, 0xe8, 0x00, 0xf6, 0x24, 0x79, 0x6e,
	0x77, 0xec, 0x25, 0xaa, 0xd6, 0x7a, 0xab, 0xc1, 0xb6, 0xb4, 0xbd, 0x34, 0x5b, 0x3a, 0x34, 0xcb,
	0xe6, 0xeb, 0x6b, 0xe8, 0x09, 0x3c, 0x56, 0xc8, 0x35, 0xb6, 0xc9, 0x85, 0xd7, 0xc7, 0xae, 0x6b,
	0x5e, 0x60, 0x5d, 0x2b, 0xec, 0x60, 0xd4, 0xb4, 0x70, 0x81, 0x57, 0xd0, 0x53, 0xd8, 0x57, 0x41,
	0x0d, 0x7b, 0xcc, 0xf6, 0x30, 0x19, 0x15, 0x5c, 0xad, 0x88, 0xe0, 0x82, 0xda, 0x9d, 0x02, 0xae,
	0x23, 0x03, 0x76, 0xb3, 0x17, 0x30, 0x62, 0xf6, 0x17, 0x62, 0x7a, 0xeb, 0x57, 0x0d, 0xf6, 0xa4,
	0x69, 0xa5, 0x79, 0x53, 0x9d, 0xe6, 0x52, 0xca, 0x61, 0x4c, 0x2c, 0xa7, 0x23, 0x42, 0xca, 0x4d,
	0x7d, 0x65, 0xe1, 0x1c, 0xd4, 0x0a, 0x53, 0x5f, 0xb9, 0x0e, 0xc9, 0xd1, 0x45, 0xa3, 0xdd, 0x9e,
	0xc9, 0xce, 0x87, 0xdd, 0x2e, 0xa6, 0x39, 0x57, 0x2d, 0x6e, 0x10, 0xc7, 0x19, 0xe4, 0x68, 0xad,
	0xe5, 0xa9, 0xa4, 0xad, 0x68, 0x36, 0x8f, 0x79, 0x92, 0xc8, 0x3e, 0x72, 0x83, 0xf3, 0x71, 0xb5,
	0x9c, 0xfe, 0x80, 0x62, 0xd7, 0x2d, 0x0d, 0xcc, 0xc5, 0x6b, 0x7b, 0xb0, 0xc0, 0xb5, 0xc5, 0x73,
	0x89, 0x39, 0x18, 0x5c, 0x2f, 0x98, 0x4a, 0xeb, 0xef, 0x1a, 0x6c, 0x89, 0x0a, 0x94, 0xa7, 0x56,
	0x34, 0xe6, 0x85, 0xf9, 0x36, 0x19, 0x39, 0x97, 0xd8, 0x73, 0x87, 0x96, 0xa5, 0xa4, 0xf3, 0xde,
	0xb3, 0xcc, 0x3b, 0x58, 0x74, 0xe8, 0x61, 0x4a, 0x75, 0x6d, 0x95, 0x53, 0xdd, 0x4b, 0xae, 0x82,
	0x8e, 0xe1, 0xa0, 0xcc, 0x11, 0x47, 0x2c, 0x6c, 0x4b, 0xd1, 0x5b, 0xab, 0x57, 0x89, 0xd3, 0x1d,
	0x12, 0x4b, 0x72, 0xcd, 0xd5, 0x31, 0x63, 0x76, 0x1f, 0x3b, 0x43, 0x26, 0xc9, 0x3d, 0x74, 0x04,
	0x46, 0x99, 0x74, 0x46, 0x98, 0xf6, 0x1c, 0xb3, 0x23, 0xd9, 0xfd, 0x55, 0x59, 0xf7, 0xda, 0x65,
	0xb8, 0x2f, 0xb9, 0x8f, 0x4a, 0xd9, 0x4b, 0xce, 0x1c, 0xb2, 0x97, 0x92, 0x79, 0xbe, 0xaa, 0x39,
	0x32, 0x7b, 0x76, 0xc7, 0x64, 0xaa, 0xd5, 0x17, 0xe8, 0x14, 0x3e, 0x2c, 0x7f, 0x12, 0x99, 0x43,
	0xe5, 0xae, 0xf8, 0xea, 0x67, 0x63, 0x39, 0x84, 0x60, 0x4b, 0x91, 0x51, 0xd1, 0x54, 0x46, 0x96,
	0x6c, 0xba, 0x5f, 0xe5, 0x4a, 0xf6, 0xfe, 0x8c, 0x0e, 0x97, 0x39, 0xea, 0x0c, 0x99, 0x70, 0x99,
	0x52, 0xfd, 0x17, 0x0d, 0x1d, 0x2d, 0x57, 0x24, 0x98, 0x5d, 0x39, 0xf4, 0x52, 0xb2, 0xbf, 0x69,
	0xe8, 0x38, 0x7b, 0x51, 0xc6, 0x2e, 0xbd, 0xe8, 0x8f, 0x07, 0xb4, 0x65, 0x12, 0x0b, 0xf7, 0xb0,
	0x32, 0xf1, 0xed, 0x42, 0x3b, 0x7b, 0xea, 0x90, 0x5c, 0x12, 0xe7, 0x8a, 0x48, 0xf6, 0xdf, 0xcd,
	0x73, 0x06, 0x9f, 0x06, 0xd1, 0xac, 0x9d, 0xf2, 0x30, 0xe0, 0x61, 0xda, 0x5e, 0xfc, 0x92, 0x69,
	0x27, 0xa9, 0x1f, 0x8e, 0xfd, 0x78, 0xdc, 0x0e, 0xa2, 0xd9, 0x2c, 0x0a, 0xcf, 0xe5, 0x17, 0x9e,
	0xff, 0x87, 0x78, 0x7d, 0x34, 0x99, 0xa6, 0x6d, 0xf1, 0x55, 0xb5, 0x23, 0x5f, 0x9c, 0xf8, 0x5c,
	0xdc, 0xfc, 0x6c, 0x12, 0xc9, 0xbf, 0x3f, 0x6c, 0x48, 0x89, 0x17, 0xff, 0x05, 0x00, 0x00, 0xff,
	0xff, 0x9c, 0xa9, 0x6d, 0xb1, 0x1a, 0x09, 0x00, 0x00,
}