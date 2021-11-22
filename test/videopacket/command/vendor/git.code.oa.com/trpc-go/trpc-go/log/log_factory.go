package log

import (
	"errors"
	"fmt"
	"path/filepath"

	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"

	"git.code.oa.com/trpc-go/trpc-go/plugin"
)

func init() {
	plugin.Register("default", DefaultLogFactory)
	RegisterWriter(OutputConsole, DefaultConsoleWriterFactory)
	RegisterWriter(OutputFile, DefaultFileWriterFactory)
	DefaultLogger = NewZapLog(defaultConfig)
}

const pluginType = "log"

// default logger
var (
	DefaultLogFactory           = &Factory{}
	DefaultConsoleWriterFactory = &ConsoleWriterFactory{}
	DefaultFileWriterFactory    = &FileWriterFactory{}

	writers = make(map[string]plugin.Factory)
	logs    = make(map[string]Logger)
)

// Register 注册日志，支持同时多个日志实现
func Register(name string, logger Logger) {
	logs[name] = logger
}

// RegisterWriter 注册日志输出writer，支持同时多个日志实现
func RegisterWriter(name string, writer plugin.Factory) {
	writers[name] = writer
}

// Get 通过日志名返回具体的实现 log.Debug使用DefaultLogger打日志，也可以使用 log.Get("name").Debug
func Get(name string) Logger {
	return logs[name]
}

// Factory 日志插件工厂 由框架启动读取配置文件 调用该工厂生成具体日志
type Factory struct {
}

// Type 日志插件类型
func (f *Factory) Type() string {
	return pluginType
}

// Decoder log
type Decoder struct {
	OutputConfig *OutputConfig
	Core         zapcore.Core
	ZapLevel     zap.AtomicLevel
}

// Decode 解析writer配置 复制一份
func (d *Decoder) Decode(conf interface{}) error {

	output, ok := conf.(**OutputConfig)
	if !ok {
		return fmt.Errorf("decoder config type:%T invalid, not **OutputConfig", conf)
	}

	*output = d.OutputConfig

	return nil
}

// Setup 启动加载log配置 并注册日志
func (f *Factory) Setup(name string, configDec plugin.Decoder) error {

	if configDec == nil {
		return errors.New("log config decoder empty")
	}

	conf, callerSkip, err := f.setupConfig(configDec)
	if err != nil {
		return err
	}

	logger := NewZapLogWithCallerSkip(conf, callerSkip)
	if logger == nil {
		return errors.New("new zap logger fail")
	}

	Register(name, logger)

	if name == "default" {
		SetLogger(logger)
	}

	return nil
}

func (f *Factory) setupConfig(configDec plugin.Decoder) (Config, int, error) {
	conf := Config{}

	err := configDec.Decode(&conf)
	if err != nil {
		return nil, 0, err
	}

	if len(conf) == 0 {
		return nil, 0, errors.New("log config output empty")
	}

	// 如果没有配置caller skip，则默认为2
	callerSkip := 2
	for i := 0; i < len(conf); i++ {
		if conf[i].CallerSkip != 0 {
			callerSkip = conf[i].CallerSkip
		}
	}
	return conf, callerSkip, nil
}

// ConsoleWriterFactory  new console writer instance
type ConsoleWriterFactory struct {
}

// Type 日志插件类型
func (f *ConsoleWriterFactory) Type() string {
	return pluginType
}

// Setup 启动加载配置 并注册console output writer
func (f *ConsoleWriterFactory) Setup(name string, configDec plugin.Decoder) error {

	if configDec == nil {
		return errors.New("console writer decoder empty")
	}
	decoder, ok := configDec.(*Decoder)
	if !ok {
		return errors.New("console writer log decoder type invalid")
	}

	conf := &OutputConfig{}
	err := decoder.Decode(&conf)
	if err != nil {
		return err
	}

	decoder.Core, decoder.ZapLevel = newConsoleCore(conf)
	return nil
}

// FileWriterFactory  new file writer instance
type FileWriterFactory struct {
}

// Type 日志插件类型
func (f *FileWriterFactory) Type() string {
	return pluginType
}

// Setup 启动加载配置 并注册file output writer
func (f *FileWriterFactory) Setup(name string, configDec plugin.Decoder) error {

	if configDec == nil {
		return errors.New("file writer decoder empty")
	}

	decoder, ok := configDec.(*Decoder)
	if !ok {
		return errors.New("file writer log decoder type invalid")
	}

	err := f.setupConfig(decoder)
	if err != nil {
		return err
	}
	return nil
}

func (f *FileWriterFactory) setupConfig(decoder *Decoder) error {
	conf := &OutputConfig{}
	err := decoder.Decode(&conf)
	if err != nil {
		return err
	}

	if conf.WriteConfig.LogPath != "" {
		conf.WriteConfig.Filename = filepath.Join(conf.WriteConfig.LogPath, conf.WriteConfig.Filename)
	}

	if conf.WriteConfig.RollType == "" {
		conf.WriteConfig.RollType = "size"
	}

	decoder.Core, decoder.ZapLevel = newFileCore(conf)
	return nil
}
