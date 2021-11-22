// Package errs trpc错误码类型，里面包含errcode errmsg，多语言通用
package errs

import (
	"fmt"
)

/*
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
	// 未明确的错误
	TrpcRetCode_TRPC_INVOKE_UNKNOWN_ERR TrpcRetCode = 999
)
*/

// trpc return code
const (
	RetOK = 0

	RetServerDecodeFail = 1
	RetServerEncodeFail = 2
	RetServerNoService  = 11
	RetServerNoFunc     = 12
	RetServerTimeout    = 21
	RetServerOverload   = 22
	RetServerSystemErr  = 31

	RetServerAuthFail     = 41
	RetServerValidateFail = 51

	RetClientTimeout      = 101
	RetClientConnectFail  = 111
	RetClientEncodeFail   = 121
	RetClientDecodeFail   = 122
	RetClientRouteErr     = 131
	RetClientNetErr       = 141
	RetClientValidateFail = 151
	RetClientCanceled     = 161

	RetUnknown = 999
)

// Err 框架错误值
var (
	ErrOK error

	ErrServerNoService = NewFrameError(RetServerNoService, "server router no service")
	ErrServerNoFunc    = NewFrameError(RetServerNoFunc, "server router no rpc method")
	ErrServerTimeout   = NewFrameError(RetServerTimeout, "server message handle timeout")
	ErrServerOverload  = NewFrameError(RetServerOverload, "server overload")
	ErrServerClose     = NewFrameError(RetServerSystemErr, "server close")

	ErrUnknown = NewFrameError(RetUnknown, "unknown error")

	ErrServerNoResponse = NewFrameError(RetOK, "server no response")
	ErrClientNoResponse = NewFrameError(RetOK, "client no response")
)

// ErrorType 错误码类型 包括框架错误码和业务错误码
const (
	ErrorTypeFramework       = 1
	ErrorTypeBusiness        = 2
	ErrorTypeCalleeFramework = 3 // client调用返回的错误码，代表是下游框架错误码
)

// Success 成功提示字符串
const (
	Success = "success"
)

// Error 错误码结构 包含 错误码类型 错误码 错误信息
type Error struct {
	Type int
	Code int32
	Msg  string
	Desc string
}

// Error 实现error接口，返回error描述
func (e *Error) Error() string {
	if e == nil {
		return Success
	}

	switch e.Type {
	case ErrorTypeFramework:
		return fmt.Sprintf("type:framework, code:%d, msg:%s", e.Code, e.Msg)
	case ErrorTypeCalleeFramework:
		return fmt.Sprintf("type:callee framework, code:%d, msg:%s", e.Code, e.Msg)
	default:
		return fmt.Sprintf("type:business, code:%d, msg:%s", e.Code, e.Msg)
	}
}

// New 创建一个error，默认为业务错误类型，提高业务开发效率
func New(code int, msg string) error {
	return &Error{
		Type: ErrorTypeBusiness,
		Code: int32(code),
		Msg:  msg,
	}
}

// NewFrameError 创建一个框架error
func NewFrameError(code int, msg string) error {
	return &Error{
		Type: ErrorTypeFramework,
		Code: int32(code),
		Msg:  msg,
		Desc: "trpc",
	}
}

// Code 通过error获取error code
func Code(e error) int {
	if e == nil {
		return 0
	}
	err, ok := e.(*Error)
	if !ok {
		return RetUnknown
	}

	if err == (*Error)(nil) {
		return 0
	}
	return int(err.Code)
}

// Msg 通过error获取error msg
func Msg(e error) string {
	if e == nil {
		return Success
	}
	err, ok := e.(*Error)
	if !ok {
		return e.Error()
	}

	if err == (*Error)(nil) {
		return Success
	}

	return err.Msg
}
