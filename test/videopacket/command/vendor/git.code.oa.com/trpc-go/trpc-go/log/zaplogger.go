package log

import (
	"fmt"
	"os"
	"strconv"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/log/rollwriter"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

var defaultConfig = []OutputConfig{
	{
		Writer:    "console",
		Level:     "debug",
		Formatter: "console",
	},
}

// core常量定义
const (
	ConsoleZapCore = "console"
	FileZapCore    = "file"
)

// Levels zapcore level
var Levels = map[string]zapcore.Level{
	"":      zapcore.DebugLevel,
	"debug": zapcore.DebugLevel,
	"info":  zapcore.InfoLevel,
	"warn":  zapcore.WarnLevel,
	"error": zapcore.ErrorLevel,
	"fatal": zapcore.FatalLevel,
}

var levelToZapLevel = map[Level]zapcore.Level{
	LevelTrace: zapcore.DebugLevel,
	LevelDebug: zapcore.DebugLevel,
	LevelInfo:  zapcore.InfoLevel,
	LevelWarn:  zapcore.WarnLevel,
	LevelError: zapcore.ErrorLevel,
	LevelFatal: zapcore.FatalLevel,
}

var zapLevelToLevel = map[zapcore.Level]Level{
	zapcore.DebugLevel: LevelDebug,
	zapcore.InfoLevel:  LevelInfo,
	zapcore.WarnLevel:  LevelWarn,
	zapcore.ErrorLevel: LevelError,
	zapcore.FatalLevel: LevelFatal,
}

// NewZapLog 创建一个trpc框架zap默认实现的logger, callerskip为2
func NewZapLog(c Config) Logger {
	return NewZapLogWithCallerSkip(c, 2)
}

// NewZapLogWithCallerSkip 创建一个trpc框架zap默认实现的logger
func NewZapLogWithCallerSkip(c Config, callerSkip int) Logger {

	cores := make([]zapcore.Core, 0, len(c))
	levels := make([]zap.AtomicLevel, 0, len(c))
	for _, o := range c {
		writer, ok := writers[o.Writer]
		if !ok {
			fmt.Printf("log writer core:%s no registered!\n", o.Writer)
			return nil
		}

		decoder := &Decoder{OutputConfig: &o}
		err := writer.Setup(o.Writer, decoder)
		if err != nil {
			fmt.Printf("log writer setup core:%s fail:%v!\n", o.Writer, err)
			return nil
		}

		cores = append(cores, decoder.Core)
		levels = append(levels, decoder.ZapLevel)
	}

	return &zapLog{
		levels: levels,
		logger: zap.New(zapcore.NewTee(cores...),
			zap.AddCallerSkip(callerSkip),
			zap.AddCaller()),
	}
}

func newEncoder(c *OutputConfig) zapcore.Encoder {
	encoderCfg := zapcore.EncoderConfig{
		TimeKey:        GetLogEncoderKey("T", c.FormatConfig.TimeKey),
		LevelKey:       GetLogEncoderKey("L", c.FormatConfig.LevelKey),
		NameKey:        GetLogEncoderKey("N", c.FormatConfig.NameKey),
		CallerKey:      GetLogEncoderKey("C", c.FormatConfig.CallerKey),
		MessageKey:     GetLogEncoderKey("M", c.FormatConfig.MessageKey),
		StacktraceKey:  GetLogEncoderKey("S", c.FormatConfig.StacktraceKey),
		LineEnding:     zapcore.DefaultLineEnding,
		EncodeLevel:    zapcore.CapitalLevelEncoder,
		EncodeTime:     NewTimeEncoder(c.FormatConfig.TimeFmt),
		EncodeDuration: zapcore.StringDurationEncoder,
		EncodeCaller:   zapcore.ShortCallerEncoder,
	}

	encoder := zapcore.NewConsoleEncoder(encoderCfg)

	switch c.Formatter {
	case "console":
		encoder = zapcore.NewConsoleEncoder(encoderCfg)
	case "json":
		encoder = zapcore.NewJSONEncoder(encoderCfg)
	default:
	}
	return encoder
}

// GetLogEncoderKey 获取用户自定义log输出字段名，没有则使用默认的
func GetLogEncoderKey(defKey, key string) string {
	if key == "" {
		return defKey
	}

	return key
}

func newConsoleCore(c *OutputConfig) (zapcore.Core, zap.AtomicLevel) {
	lvl := zap.NewAtomicLevelAt(Levels[c.Level])
	return zapcore.NewCore(
		newEncoder(c),
		zapcore.Lock(os.Stdout),
		lvl), lvl
}

func newFileCore(c *OutputConfig) (zapcore.Core, zap.AtomicLevel) {
	var ws zapcore.WriteSyncer

	if c.WriteConfig.RollType == "size" {
		// 按大小滚动
		writer, _ := rollwriter.NewRollWriter(
			c.WriteConfig.Filename,
			rollwriter.WithMaxAge(c.WriteConfig.MaxAge),
			rollwriter.WithMaxBackups(c.WriteConfig.MaxBackups),
			rollwriter.WithCompress(c.WriteConfig.Compress),
			rollwriter.WithMaxSize(c.WriteConfig.MaxSize),
		)
		ws = zapcore.AddSync(writer)
	} else {
		// 按时间滚动
		writer, _ := rollwriter.NewRollWriter(
			c.WriteConfig.Filename,
			rollwriter.WithMaxAge(c.WriteConfig.MaxAge),
			rollwriter.WithMaxBackups(c.WriteConfig.MaxBackups),
			rollwriter.WithCompress(c.WriteConfig.Compress),
			rollwriter.WithRotationTime(c.WriteConfig.TimeUnit.Format()),
		)
		ws = zapcore.AddSync(writer)
	}

	lvl := zap.NewAtomicLevelAt(Levels[c.Level])

	return zapcore.NewCore(
		newEncoder(c),
		ws, lvl,
	), lvl
}

// NewTimeEncoder 创建时间格式encoder
func NewTimeEncoder(format string) zapcore.TimeEncoder {
	switch format {
	case "":
		return func(t time.Time, enc zapcore.PrimitiveArrayEncoder) {
			enc.AppendByteString(DefaultTimeFormat(t))
		}
	case "seconds":
		return zapcore.EpochTimeEncoder
	case "milliseconds":
		return zapcore.EpochMillisTimeEncoder
	case "nanoseconds":
		return zapcore.EpochNanosTimeEncoder
	default:
	}
	return func(t time.Time, enc zapcore.PrimitiveArrayEncoder) {
		enc.AppendString(CustomTimeFormat(t, format))
	}
}

// CustomTimeFormat 自定义时间格式
func CustomTimeFormat(t time.Time, format string) string {
	return t.Format(format)
}

// DefaultTimeFormat 默认时间格式
func DefaultTimeFormat(t time.Time) []byte {
	t = t.Local()
	year, month, day := t.Date()
	hour, minute, second := t.Clock()
	micros := t.Nanosecond() / 1000

	buf := make([]byte, 23)
	buf[0] = byte((year/1000)%10) + '0'
	buf[1] = byte((year/100)%10) + '0'
	buf[2] = byte((year/10)%10) + '0'
	buf[3] = byte(year%10) + '0'
	buf[4] = '-'
	buf[5] = byte((month)/10) + '0'
	buf[6] = byte((month)%10) + '0'
	buf[7] = '-'
	buf[8] = byte((day)/10) + '0'
	buf[9] = byte((day)%10) + '0'
	buf[10] = ' '
	buf[11] = byte((hour)/10) + '0'
	buf[12] = byte((hour)%10) + '0'
	buf[13] = ':'
	buf[14] = byte((minute)/10) + '0'
	buf[15] = byte((minute)%10) + '0'
	buf[16] = ':'
	buf[17] = byte((second)/10) + '0'
	buf[18] = byte((second)%10) + '0'
	buf[19] = '.'
	buf[20] = byte((micros/100000)%10) + '0'
	buf[21] = byte((micros/10000)%10) + '0'
	buf[22] = byte((micros/1000)%10) + '0'
	return buf
}

// ZapLogWrapper 是对 zapLogger 的代理，引入 ZapLogWrapper 的原因见 [issue](https://git.code.oa.com/trpc-go/trpc-go/issues/260)
// 通过引入 ZapLogWrapper 这个代理，使 debug 系列函数的调用增加一层，让 caller 信息能够正确的设置
type ZapLogWrapper struct {
	l *zapLog
}

// GetLogger 返回内部的zapLog
func (z *ZapLogWrapper) GetLogger() Logger {
	return z.l
}

// Trace logs to TRACE log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Trace(args ...interface{}) {
	z.l.Trace(args...)
}

// Tracef logs to TRACE log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Tracef(format string, args ...interface{}) {
	z.l.Tracef(format, args...)
}

// Debug logs to DEBUG log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Debug(args ...interface{}) {
	z.l.Debug(args...)
}

// Debugf logs to DEBUG log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Debugf(format string, args ...interface{}) {
	z.l.Debugf(format, args...)
}

// Info logs to INFO log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Info(args ...interface{}) {
	z.l.Info(args...)
}

// Infof logs to INFO log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Infof(format string, args ...interface{}) {
	z.l.Infof(format, args...)
}

// Warn logs to WARNING log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Warn(args ...interface{}) {
	z.l.Warn(args...)
}

// Warnf logs to WARNING log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Warnf(format string, args ...interface{}) {
	z.l.Warnf(format, args...)
}

// Error logs to ERROR log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Error(args ...interface{}) {
	z.l.Error(args...)
}

// Errorf logs to ERROR log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Errorf(format string, args ...interface{}) {
	z.l.Errorf(format, args...)
}

// Fatal logs to FATAL log, Arguments are handled in the manner of fmt.Print
func (z *ZapLogWrapper) Fatal(args ...interface{}) {
	z.l.Fatal(args...)
}

// Fatalf logs to FATAL log, Arguments are handled in the manner of fmt.Printf
func (z *ZapLogWrapper) Fatalf(format string, args ...interface{}) {
	z.l.Fatalf(format, args...)
}

// SetLevel 设置输出端日志级别
func (z *ZapLogWrapper) SetLevel(output string, level Level) {
	z.l.SetLevel(output, level)
}

// GetLevel 获取输出端日志级别
func (z *ZapLogWrapper) GetLevel(output string) Level {
	return z.l.GetLevel(output)
}

// WithFields 设置一些业务自定义数据到每条log里:比如uid，imei等, 每个请求入口设置，并生成一个新的logger，后续使用新的logger来打日志 fields 必须kv成对出现
func (z *ZapLogWrapper) WithFields(fields ...string) Logger {
	return z.l.WithFields(fields...)
}

type zapLog struct {
	levels []zap.AtomicLevel
	logger *zap.Logger
}

// WithFields 设置一些业务自定义数据到每条log里:比如uid，imei等, 每个请求入口设置，并生成一个新的logger，后续使用新的logger来打日志 fields 必须kv成对出现
func (l *zapLog) WithFields(fields ...string) Logger {

	zapfields := make([]zap.Field, len(fields)/2)
	for index := range zapfields {
		zapfields[index] = zap.String(fields[2*index], fields[2*index+1])
	}

	// 使用 ZapLogWrapper 代理，这样返回的 Logger 被调用时，调用栈层数和使用 Debug 系列函数一致，caller 信息能够正确的设置
	return &ZapLogWrapper{l: &zapLog{logger: l.logger.With(zapfields...)}}
}

// Trace logs to TRACE log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Trace(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.DebugLevel) {
		l.logger.Debug(fmt.Sprint(args...))
	}
}

// Tracef logs to TRACE log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Tracef(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.DebugLevel) {
		l.logger.Debug(fmt.Sprintf(format, args...))
	}
}

// Debug logs to DEBUG log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Debug(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.DebugLevel) {
		l.logger.Debug(fmt.Sprint(args...))
	}
}

// Debugf logs to DEBUG log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Debugf(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.DebugLevel) {
		l.logger.Debug(fmt.Sprintf(format, args...))
	}
}

// Info logs to INFO log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Info(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.InfoLevel) {
		l.logger.Info(fmt.Sprint(args...))
	}
}

// Infof logs to INFO log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Infof(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.InfoLevel) {
		l.logger.Info(fmt.Sprintf(format, args...))
	}
}

// Warn logs to WARNING log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Warn(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.WarnLevel) {
		l.logger.Warn(fmt.Sprint(args...))
	}
}

// Warnf logs to WARNING log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Warnf(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.WarnLevel) {
		l.logger.Warn(fmt.Sprintf(format, args...))
	}
}

// Error logs to ERROR log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Error(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.ErrorLevel) {
		l.logger.Error(fmt.Sprint(args...))
	}
}

// Errorf logs to ERROR log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Errorf(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.ErrorLevel) {
		l.logger.Error(fmt.Sprintf(format, args...))
	}
}

// Fatal logs to FATAL log, Arguments are handled in the manner of fmt.Print
func (l *zapLog) Fatal(args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.FatalLevel) {
		l.logger.Fatal(fmt.Sprint(args...))
	}
}

// Fatalf logs to FATAL log, Arguments are handled in the manner of fmt.Printf
func (l *zapLog) Fatalf(format string, args ...interface{}) {
	if l.logger.Core().Enabled(zapcore.FatalLevel) {
		l.logger.Fatal(fmt.Sprintf(format, args...))
	}
}

// SetLevel 设置输出端日志级别
func (l *zapLog) SetLevel(output string, level Level) {
	i, e := strconv.Atoi(output)
	if e != nil {
		return
	}
	if i < 0 || i >= len(l.levels) {
		return
	}
	l.levels[i].SetLevel(levelToZapLevel[level])
}

// GetLevel 获取输出端日志级别
func (l *zapLog) GetLevel(output string) Level {
	i, e := strconv.Atoi(output)
	if e != nil {
		return LevelDebug
	}
	if i < 0 || i >= len(l.levels) {
		return LevelDebug
	}
	return zapLevelToLevel[l.levels[i].Level()]
}
