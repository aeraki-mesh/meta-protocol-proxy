package log

import (
	"time"

	yaml "gopkg.in/yaml.v3"
)

// output name
const (
	OutputConsole = "console"
	OutputFile    = "file"
)

// Config log config 每个log可以支持多个output
type Config []OutputConfig

// OutputConfig 每个output的配置 包括console file remote
type OutputConfig struct {
	// Writer 日志输出端 (console, file)
	Writer      string
	WriteConfig WriteConfig `yaml:"writer_config"`

	// Formatter 日志输出格式 (console, json)
	Formatter    string
	FormatConfig FormatConfig `yaml:"formatter_config"`

	// RemoteConfig 远程日志格式 配置格式业务随便定 由第三方组件自己注册
	RemoteConfig yaml.Node `yaml:"remote_config"`

	// Level 控制日志级别 debug info error
	Level string

	// CallerSkip 控制log函数嵌套深度
	CallerSkip int `yaml:"caller_skip"`
}

// WriteConfig 本地文件的配置
type WriteConfig struct {
	// LogPath 日志路径名  /usr/local/trpc/log/
	LogPath string `yaml:"log_path"`
	// Filename 日志路径文件名  trpc.log
	Filename string `yaml:"filename"`
	// RollType 文件滚动类型，size-按大小分割文件，time-按时间分割文件，默认按大小分割
	RollType string `yaml:"roll_type"`
	// MaxAge 日志最大保留时间, 天
	MaxAge int `yaml:"max_age"`
	// MaxBackups 日志最大文件数
	MaxBackups int `yaml:"max_backups"`
	// Compress 日志文件是否压缩
	Compress bool `yaml:"compress"`

	// 以下参数按大小分割时才有效
	// MaxSize 日志文件最大大小（单位MB）
	MaxSize int `yaml:"max_size"`

	// 以下参数按时间分割时才有效
	// TimeUnit 按时间分割文件的时间单位
	// 支持year/month/day/hour/minute, 默认为day
	TimeUnit TimeUnit `yaml:"time_unit"`
}

// FormatConfig 日志格式配置
type FormatConfig struct {
	// TimeFmt 日志输出时间格式，空默认为"2006-01-02 15:04:05.000"
	TimeFmt string `yaml:"time_fmt"`

	// TimeKey 日志输出时间key， 默认为"T"
	TimeKey string `yaml:"time_key"`
	// LevelKey 日志级别输出key， 默认为"L"
	LevelKey string `yaml:"level_key"`
	// NameKey 日志名称key， 默认为"N"
	NameKey string `yaml:"name_key"`
	// CallerKey 日志输出调用者key， 默认"C"
	CallerKey string `yaml:"caller_key"`
	// MessageKey 日志输出消息体key，默认"M"
	MessageKey string `yaml:"message_key"`
	// StacktraceKey 日志输出堆栈trace key， 默认"S"
	StacktraceKey string `yaml:"stacktrace_key"`
}

// TimeUnit 文件按时间分割的时间单位，支持：minute/hour/day/month/year
type TimeUnit string

// 时间单位配置字段
const (
	Minute = "minute"
	Hour   = "hour"
	Day    = "day"
	Month  = "month"
	Year   = "year"
)

// Format 返回时间单位的格式字符串（c风格），默认返回day的格式字符串
func (t TimeUnit) Format() string {
	switch t {
	case Minute:
		return ".%Y%m%d%H%M"
	case Hour:
		return ".%Y%m%d%H"
	case Day:
		return ".%Y%m%d"
	case Month:
		return ".%Y%m"
	case Year:
		return ".%Y"
	default:
		return ".%Y%m%d"
	}
}

// RotationGap 返回时间单位对应的时间值，默认返回一天的时间
func (t TimeUnit) RotationGap() time.Duration {
	switch t {
	case Minute:
		return time.Minute
	case Hour:
		return time.Hour
	case Day:
		return time.Hour * 24
	case Month:
		return time.Hour * 24 * 30
	case Year:
		return time.Hour * 24 * 365
	default:
		return time.Hour * 24
	}
}
