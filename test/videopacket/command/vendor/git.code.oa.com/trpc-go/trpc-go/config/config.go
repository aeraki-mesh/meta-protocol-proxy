// Package config 通用配置接口
package config

import (
	"context"
	"encoding/json"
	"errors"
	"strconv"
	"sync"

	"github.com/BurntSushi/toml"

	yaml "gopkg.in/yaml.v3"
)

// ErrConfigNotSupport 尚未支持
var ErrConfigNotSupport = errors.New("trpc/config: not support")

// GetString 根据key获取string类型的值
func GetString(key string) (string, error) {
	val, err := globalKV.Get(context.Background(), key)
	if err != nil {
		return "", err
	}
	return val.Value(), nil
}

// GetInt 根据key获取Int类型的值
func GetInt(key string) (int, error) {
	val, err := globalKV.Get(context.Background(), key)
	if err != nil {
		return 0, err
	}

	return strconv.Atoi(val.Value())
}

// GetWithUnmarshal 根据key获取特定编码的数据结构
func GetWithUnmarshal(key string, val interface{}, unmarshalName string) error {
	v, err := globalKV.Get(context.Background(), key)
	if err != nil {
		return err
	}
	return GetUnmarshaler(unmarshalName).Unmarshal([]byte(v.Value()), val)
}

// GetJSON 根据key获取Json类型复合数据
func GetJSON(key string, val interface{}) error {
	return GetWithUnmarshal(key, val, "json")
}

// GetYAML 根据key获取YAML类型复合数据
func GetYAML(key string, val interface{}) error {
	return GetWithUnmarshal(key, val, "yaml")
}

// GetTOML 根据key获取TOML类型复合数据
func GetTOML(key string, val interface{}) error {
	return GetWithUnmarshal(key, val, "toml")
}

// Unmarshaler 配置内容解析抽象
type Unmarshaler interface {
	Unmarshal(data []byte, value interface{}) error
}

var (
	unmarshalers = make(map[string]Unmarshaler)
)

// YamlUnmarshaler yaml解码
type YamlUnmarshaler struct{}

// Unmarshal yaml 解压
func (yu *YamlUnmarshaler) Unmarshal(data []byte, val interface{}) error {
	return yaml.Unmarshal(data, val)
}

// JSONUnmarshaler json解码
type JSONUnmarshaler struct{}

// Unmarshal json 解码
func (ju *JSONUnmarshaler) Unmarshal(data []byte, val interface{}) error {
	return json.Unmarshal(data, val)
}

// TomlUnmarshaler toml解码
type TomlUnmarshaler struct{}

// Unmarshal toml解码
func (tu *TomlUnmarshaler) Unmarshal(data []byte, val interface{}) error {
	return toml.Unmarshal(data, val)
}

func init() {
	RegisterUnmarshaler("yaml", &YamlUnmarshaler{})
	RegisterUnmarshaler("json", &JSONUnmarshaler{})
	RegisterUnmarshaler("toml", &TomlUnmarshaler{})
}

// RegisterUnmarshaler 注册Unmarshaler
func RegisterUnmarshaler(name string, us Unmarshaler) {
	unmarshalers[name] = us
}

// GetUnmarshaler 获取Unmarshaler
func GetUnmarshaler(name string) Unmarshaler {
	return unmarshalers[name]
}

var (
	configMap = make(map[string]KVConfig)
)

// KVConfig kv配置
type KVConfig interface {
	KV
	Watcher
	Name() string
}

// Register 注册kvconfig
func Register(c KVConfig) {
	lock.Lock()
	configMap[c.Name()] = c
	lock.Unlock()
}

// Get 根据名字使用kvconfig
func Get(name string) KVConfig {
	lock.RLock()
	c := configMap[name]
	lock.RUnlock()
	return c
}

// GlobalKV 获取配置中心KV实例
func GlobalKV() KV {
	return globalKV
}

// SetGlobalKV 设置配置中心KV实例
func SetGlobalKV(kv KV) {
	globalKV = kv
}

// EventType 监听配置变更的事件类型
type EventType uint8

const (
	// EventTypeNull 空事件
	EventTypeNull EventType = 0
	// EventTypePut 设置或更新配置事件
	EventTypePut EventType = 1
	// EventTypeDel 删除配置项事件
	EventTypeDel EventType = 2
)

// Response 配置中心响应
type Response interface {
	// Value 获取配置项对应的值
	Value() string
	// MetaData 额外元数据信息
	// 配置Option选项，可用于承载不同配置中心的额外功能实现，例如namespace,group,租约等概念
	MetaData() map[string]string
	// Event 获取Watch事件类型
	Event() EventType
}

// KV 配置中心键值对接口
type KV interface {
	// Put 设置或更新配置项key对应的值
	Put(ctx context.Context, key, val string, opts ...Option) error
	// Get 获取配置项key对应的值
	Get(ctx context.Context, key string, opts ...Option) (Response, error)
	// Del 删除配置项key
	Del(ctx context.Context, key string, opts ...Option) error
}

// Watcher 配置中心Watch事件接口
type Watcher interface {
	// Watch 监听配置项key的变更事件
	Watch(ctx context.Context, key string, opts ...Option) (<-chan Response, error)
}

var globalKV KV = &noopKV{}

type noopKV struct{}

// Put 设置或更新配置项key对应的值
func (kv *noopKV) Put(ctx context.Context, key, val string, opts ...Option) error {
	return nil
}

// Get 获取配置项key对应的值
func (kv *noopKV) Get(ctx context.Context, key string, opts ...Option) (Response, error) {
	return nil, ErrConfigNotSupport
}

// Del 删除配置项key
func (kv *noopKV) Del(ctx context.Context, key string, opts ...Option) error {
	return nil
}

// Loader 解析配置的通用接口
type Loader interface {
	//TODO:complete the design of interface based on tconf. Passing a struct may not be a good choice
	Load(string) (Config, error)
	Reload(string) error
}

// Config 配置通用接口
type Config interface {
	Load() error
	Reload()
	Get(string, interface{}) interface{}
	Unmarshal(interface{}) error
	IsSet(string) bool
	GetInt(string, int) int
	GetInt32(string, int32) int32
	GetInt64(string, int64) int64
	GetUint(string, uint) uint
	GetUint32(string, uint32) uint32
	GetUint64(string, uint64) uint64
	GetFloat32(string, float32) float32
	GetFloat64(string, float64) float64
	GetString(string, string) string
	GetBool(string, bool) bool
	Bytes() []byte
}

// ProviderCallback provider内容变更事件回调函数
type ProviderCallback func(string, []byte)

// DataProvider 通用内容源接口
// 通过实现Name、Read、Watch等方法，就能从任意的
// 内容源（file、TConf、ETCD、configmap）中读取配
// 置并通过编解码器解析为可处理的标准格式（JSON、TOML、YAML）等
type DataProvider interface {
	//TODO:add ability to watch
	Name() string
	Read(string) ([]byte, error)
	Watch(ProviderCallback)
}

// Codec 编解码器
type Codec interface {
	Name() string
	Unmarshal([]byte, interface{}) error
}

var providerMap = make(map[string]DataProvider)

// RegisterProvider 注册配置服务提供者组件
func RegisterProvider(p DataProvider) {
	providerMap[p.Name()] = p
}

// GetProvider 根据名字获取provider
func GetProvider(name string) DataProvider {
	return providerMap[name]
}

var (
	codecMap = make(map[string]Codec)
	lock     = sync.RWMutex{}
)

// RegisterCodec 注册编解码器
func RegisterCodec(c Codec) {
	lock.Lock()
	codecMap[c.Name()] = c
	lock.Unlock()
}

// GetCodec 根据名字获取Codec
func GetCodec(name string) Codec {
	lock.RLock()
	c := codecMap[name]
	lock.RUnlock()
	return c
}

// Load 根据参数读取指定配置
func Load(path string, opts ...LoadOption) (Config, error) {
	return DefaultConfigLoader.Load(path, opts...)
}
