// Package log 提供框架和应用日志输出能力
package log

import (
	"context"
	"os"

	"git.code.oa.com/trpc-go/trpc-go/codec"
)

func init() {
	traceEnabled = traceEnableFromEnv()
}

// 环境变量控制trace级别日志输出
// TRPC_LOG_TRACE=1 开启trace输出
const logEnv = "TRPC_LOG_TRACE"

var traceEnabled = false

// 读取环境变量,判断是否开启Trace
// 默认关闭
// 为空或者为0，关闭Trace
// 非空且非0，开启Trace
func traceEnableFromEnv() bool {
	switch os.Getenv(logEnv) {
	case "":
		fallthrough
	case "0":
		return false
	default:
		return true
	}
}

// EnableTrace 开启trace级别日志
func EnableTrace() {
	traceEnabled = true
}

// DefaultLogger 默认的logger，通过配置文件key=default来设置, 默认使用console输出
var DefaultLogger Logger

// SetLogger 设置默认logger
func SetLogger(logger Logger) {
	DefaultLogger = logger
}

// SetLevel 设置不同的输出对应的日志级别, output为输出数组下标 "0" "1" "2"
func SetLevel(output string, level Level) {
	DefaultLogger.SetLevel(output, level)
}

// GetLevel 获取不同输出对应的日志级别
func GetLevel(output string) Level {
	return DefaultLogger.GetLevel(output)
}

// WithFields 设置一些业务自定义数据到每条log里:比如uid，imei等, fields 必须kv成对出现
func WithFields(fields ...string) Logger {
	return DefaultLogger.WithFields(fields...)
}

// WithFieldsContext 以当前 context logger 为基础，增加设置一些业务自定义数据到每条log里:比如uid，imei等, fields 必须kv成对出现
func WithFieldsContext(ctx context.Context, fields ...string) Logger {
	logger, ok := codec.Message(ctx).Logger().(Logger)
	if !ok {
		return WithFields(fields...)
	}
	return logger.WithFields(fields...)
}

// Trace logs to TRACE log. Arguments are handled in the manner of fmt.Print.
func Trace(args ...interface{}) {
	if traceEnabled {
		DefaultLogger.Trace(args...)
	}
}

// Tracef logs to TRACE log. Arguments are handled in the manner of fmt.Printf.
func Tracef(format string, args ...interface{}) {
	if traceEnabled {
		DefaultLogger.Tracef(format, args...)
	}
}

// TraceContext logs to TRACE log. Arguments are handled in the manner of fmt.Print.
func TraceContext(ctx context.Context, args ...interface{}) {
	if !traceEnabled {
		return
	}

	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Trace(args...)
			return
		}
		l.l.Trace(args...)
	case Logger:
		l.Trace(args...)
	default:
		DefaultLogger.Trace(args...)
	}
}

// TraceContextf logs to TRACE log. Arguments are handled in the manner of fmt.Printf.
func TraceContextf(ctx context.Context, format string, args ...interface{}) {
	if !traceEnabled {
		return
	}

	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Tracef(format, args...)
			return
		}
		l.l.Tracef(format, args...)
	case Logger:
		l.Tracef(format, args...)
	default:
		DefaultLogger.Tracef(format, args...)
	}
}

// Debug logs to DEBUG log. Arguments are handled in the manner of fmt.Print.
func Debug(args ...interface{}) {
	DefaultLogger.Debug(args...)
}

// Debugf logs to DEBUG log. Arguments are handled in the manner of fmt.Printf.
func Debugf(format string, args ...interface{}) {
	DefaultLogger.Debugf(format, args...)
}

// Info logs to INFO log. Arguments are handled in the manner of fmt.Print.
func Info(args ...interface{}) {
	DefaultLogger.Info(args...)
}

// Infof logs to INFO log. Arguments are handled in the manner of fmt.Printf.
func Infof(format string, args ...interface{}) {
	DefaultLogger.Infof(format, args...)
}

// Warn logs to WARNING log. Arguments are handled in the manner of fmt.Print.
func Warn(args ...interface{}) {
	DefaultLogger.Warn(args...)
}

// Warnf logs to WARNING log. Arguments are handled in the manner of fmt.Printf.
func Warnf(format string, args ...interface{}) {
	DefaultLogger.Warnf(format, args...)
}

// Error logs to ERROR log. Arguments are handled in the manner of fmt.Print.
func Error(args ...interface{}) {
	DefaultLogger.Error(args...)
}

// Errorf logs to ERROR log. Arguments are handled in the manner of fmt.Printf.
func Errorf(format string, args ...interface{}) {
	DefaultLogger.Errorf(format, args...)
}

// Fatal logs to ERROR log. Arguments are handled in the manner of fmt.Print.
// that all Fatal logs will exit with os.Exit(1).
// Implementations may also call os.Exit() with a non-zero exit code.
func Fatal(args ...interface{}) {
	DefaultLogger.Fatal(args...)
}

// Fatalf logs to ERROR log. Arguments are handled in the manner of fmt.Printf.
func Fatalf(format string, args ...interface{}) {
	DefaultLogger.Fatalf(format, args...)
}

// WithContextFields 设置一些业务自定义数据到每条log里:比如uid，imei等, fields 必须kv成对出现
func WithContextFields(ctx context.Context, fields ...string) {
	msg := codec.Message(ctx)
	logger, ok := msg.Logger().(Logger)
	if ok && logger != nil {
		logger = logger.WithFields(fields...)
	} else {
		logger = DefaultLogger.WithFields(fields...)
	}
	msg.WithLogger(logger)
}

// DebugContext logs to DEBUG log. Arguments are handled in the manner of fmt.Print.
func DebugContext(ctx context.Context, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Debug(args...)
			return
		}
		l.l.Debug(args...)
	case Logger:
		l.Debug(args...)
	default:
		DefaultLogger.Debug(args...)
	}
}

// DebugContextf logs to DEBUG log. Arguments are handled in the manner of fmt.Printf.
func DebugContextf(ctx context.Context, format string, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Debugf(format, args...)
			return
		}
		l.l.Debugf(format, args...)
	case Logger:
		l.Debugf(format, args...)
	default:
		DefaultLogger.Debugf(format, args...)
	}
}

// InfoContext logs to INFO log. Arguments are handled in the manner of fmt.Print.
func InfoContext(ctx context.Context, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Info(args...)
			return
		}
		l.l.Info(args...)
	case Logger:
		l.Info(args...)
	default:
		DefaultLogger.Info(args...)
	}
}

// InfoContextf logs to INFO log. Arguments are handled in the manner of fmt.Printf.
func InfoContextf(ctx context.Context, format string, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Infof(format, args...)
			return
		}
		l.l.Infof(format, args...)
	case Logger:
		l.Infof(format, args...)
	default:
		DefaultLogger.Infof(format, args...)
	}
}

// WarnContext logs to WARNING log. Arguments are handled in the manner of fmt.Print.
func WarnContext(ctx context.Context, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Warn(args...)
			return
		}
		l.l.Warn(args...)
	case Logger:
		l.Warn(args...)
	default:
		DefaultLogger.Warn(args...)
	}
}

// WarnContextf logs to WARNING log. Arguments are handled in the manner of fmt.Printf.
func WarnContextf(ctx context.Context, format string, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Warnf(format, args...)
			return
		}
		l.l.Warnf(format, args...)
	case Logger:
		l.Warnf(format, args...)
	default:
		DefaultLogger.Warnf(format, args...)
	}
}

// ErrorContext logs to ERROR log. Arguments are handled in the manner of fmt.Print.
func ErrorContext(ctx context.Context, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Error(args...)
			return
		}
		l.l.Error(args...)
	case Logger:
		l.Error(args...)
	default:
		DefaultLogger.Error(args...)
	}
}

// ErrorContextf logs to ERROR log. Arguments are handled in the manner of fmt.Printf.
func ErrorContextf(ctx context.Context, format string, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Errorf(format, args...)
			return
		}
		l.l.Errorf(format, args...)
	case Logger:
		l.Errorf(format, args...)
	default:
		DefaultLogger.Errorf(format, args...)
	}
}

// FatalContext logs to ERROR log. Arguments are handled in the manner of fmt.Print.
// that all Fatal logs will exit with os.Exit(1).
// Implementations may also call os.Exit() with a non-zero exit code.
func FatalContext(ctx context.Context, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Fatal(args...)
			return
		}
		l.l.Fatal(args...)
	case Logger:
		l.Fatal(args...)
	default:
		DefaultLogger.Fatal(args...)
	}
}

// FatalContextf logs to ERROR log. Arguments are handled in the manner of fmt.Printf.
func FatalContextf(ctx context.Context, format string, args ...interface{}) {
	switch l := codec.Message(ctx).Logger().(type) {
	case *ZapLogWrapper:
		// 保护 l 或者 l.l 不可为空
		if l == nil || l.l == nil {
			DefaultLogger.Fatalf(format, args...)
			return
		}
		l.l.Fatalf(format, args...)
	case Logger:
		l.Fatalf(format, args...)
	default:
		DefaultLogger.Fatalf(format, args...)
	}
}
