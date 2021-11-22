package config

import (
	"encoding/json"
	"errors"
	"fmt"
	"strings"
	"sync"

	"git.code.oa.com/trpc-go/trpc-go/log"

	"github.com/spf13/cast"
	"gopkg.in/yaml.v3"
)

var (
	// ErrConfigNotExist 配置不存在
	ErrConfigNotExist = errors.New("trpc/config: config not exist")
	// ErrProviderNotExist provider不存在
	ErrProviderNotExist = errors.New("trpc/config: provider not exist")
	// ErrCodecNotExist codec不存在
	ErrCodecNotExist = errors.New("trpc/config: codec not exist")
)

func init() {
	RegisterCodec(&YamlCodec{})
	RegisterCodec(&JSONCodec{})
}

// LoadOption 配置加载选项
type LoadOption func(*TrpcConfig)

// TrpcConfigLoader 创建一个Config实例
type TrpcConfigLoader struct {
	configMap map[string]Config
	rwl       sync.RWMutex
}

// Load 根据参数加载指定配置
func (loader *TrpcConfigLoader) Load(path string, opts ...LoadOption) (Config, error) {
	yc := newTrpcConfig(path)
	for _, o := range opts {
		o(yc)
	}

	if yc.decoder == nil {
		return nil, ErrCodecNotExist
	}

	if yc.p == nil {
		return nil, ErrProviderNotExist
	}

	key := fmt.Sprintf("%s.%s.%s", yc.decoder.Name(), yc.p.Name(), path)
	loader.rwl.RLock()
	if c, ok := loader.configMap[key]; ok {
		loader.rwl.RUnlock()
		return c, nil
	}
	loader.rwl.RUnlock()

	err := yc.Load()
	if err != nil {
		return nil, err
	}

	loader.rwl.Lock()
	loader.configMap[key] = yc
	loader.rwl.Unlock()

	yc.p.Watch(func(p string, data []byte) {
		if p == path {
			loader.rwl.Lock()
			delete(loader.configMap, key)
			loader.rwl.Unlock()
		}
	})

	return yc, nil
}

// Reload 重新加载
func (loader *TrpcConfigLoader) Reload(path string, opts ...LoadOption) error {
	yc := newTrpcConfig(path)
	for _, o := range opts {
		o(yc)
	}
	key := fmt.Sprintf("%s.%s.%s", yc.decoder.Name(), yc.p.Name(), path)
	loader.rwl.RLock()
	if config, ok := loader.configMap[key]; ok {
		loader.rwl.RUnlock()
		config.Reload()
		return nil
	}
	loader.rwl.RUnlock()
	return ErrConfigNotExist
}

func newTrpcConfigLoad() *TrpcConfigLoader {
	return &TrpcConfigLoader{configMap: map[string]Config{}, rwl: sync.RWMutex{}}
}

// DefaultConfigLoader 默认配置加载器
var DefaultConfigLoader = newTrpcConfigLoad()

// YamlCodec 解码Yaml
type YamlCodec struct{}

// Name yaml codec
func (*YamlCodec) Name() string {
	return "yaml"
}

// Unmarshal yaml decode
func (c *YamlCodec) Unmarshal(in []byte, out interface{}) error {
	return yaml.Unmarshal(in, out)
}

// JSONCodec json codec
type JSONCodec struct{}

// Name yaml codec
func (*JSONCodec) Name() string {
	return "json"
}

// Unmarshal yaml decode
func (c *JSONCodec) Unmarshal(in []byte, out interface{}) error {
	return json.Unmarshal(in, out)
}

// TrpcConfig 解析yaml类型的配置文件
type TrpcConfig struct {
	p             DataProvider
	unmarshedData interface{}
	path          string
	decoder       Codec
	rawData       []byte
}

func (c *TrpcConfig) find(key string) (interface{}, error) {
	subkeys := c.parseKey(key)
	return c.locateSubkey(subkeys)
}

// Get 根据key读取配置
func (c *TrpcConfig) Get(key string, defaultValue interface{}) interface{} {
	if v, err := c.find(key); err == nil {
		return v
	}

	return defaultValue
}

// Bytes 获得原始配置
func (c *TrpcConfig) Bytes() []byte {
	return c.rawData
}

func (c *TrpcConfig) findWithDefaultValue(key string, defaultValue interface{}) interface{} {
	v, err := c.find(key)
	if err != nil {
		return defaultValue
	}

	switch defaultValue.(type) {
	case bool:
		v, err = cast.ToBoolE(v)
	case string:
		v, err = cast.ToStringE(v)
	case int:
		v, err = cast.ToIntE(v)
	case int32:
		v, err = cast.ToInt32E(v)
	case int64:
		v, err = cast.ToInt64E(v)
	case uint:
		v, err = cast.ToUintE(v)
	case uint32:
		v, err = cast.ToUint32E(v)
	case uint64:
		v, err = cast.ToUint64E(v)
	case float64:
		v, err = cast.ToFloat64E(v)
	case float32:
		v, err = cast.ToFloat32E(v)
	default:
	}

	if err != nil {
		return defaultValue
	}

	return v
}

// GetInt 根据key读取int类型配置
func (c *TrpcConfig) GetInt(key string, defaultValue int) int {
	return cast.ToInt(c.findWithDefaultValue(key, defaultValue))
}

// GetInt32 根据key读取int32类型配置
func (c *TrpcConfig) GetInt32(key string, defaultValue int32) int32 {
	return cast.ToInt32(c.findWithDefaultValue(key, defaultValue))
}

// GetInt64 根据key读取int64类型配置
func (c *TrpcConfig) GetInt64(key string, defaultValue int64) int64 {
	return cast.ToInt64(c.findWithDefaultValue(key, defaultValue))
}

// GetUint 根据key读取int类型配置
func (c *TrpcConfig) GetUint(key string, defaultValue uint) uint {
	return cast.ToUint(c.findWithDefaultValue(key, defaultValue))
}

// GetUint32 根据key读取uint32类型配置
func (c *TrpcConfig) GetUint32(key string, defaultValue uint32) uint32 {
	return cast.ToUint32(c.findWithDefaultValue(key, defaultValue))
}

// GetUint64 根据key读取uint64类型配置
func (c *TrpcConfig) GetUint64(key string, defaultValue uint64) uint64 {
	return cast.ToUint64(c.findWithDefaultValue(key, defaultValue))
}

// GetFloat64 根据key读取float64类型配置
func (c *TrpcConfig) GetFloat64(key string, defaultValue float64) float64 {
	return cast.ToFloat64(c.findWithDefaultValue(key, defaultValue))
}

// GetFloat32 根据key读取float32类型配置
func (c *TrpcConfig) GetFloat32(key string, defaultValue float32) float32 {
	return cast.ToFloat32(c.findWithDefaultValue(key, defaultValue))
}

// GetBool 根据key读取bool类型配置
func (c *TrpcConfig) GetBool(key string, defaultValue bool) bool {
	return cast.ToBool(c.findWithDefaultValue(key, defaultValue))
}

// IsSet 根据key判断配置是否存在
func (c *TrpcConfig) IsSet(key string) bool {
	subkeys := c.parseKey(key)
	_, err := c.locateSubkey(subkeys)
	if err != nil {
		return false
	}
	return true
}

func (c *TrpcConfig) locateSubkey(subkeys []string) (interface{}, error) {
	return c.search(cast.ToStringMap(c.unmarshedData), subkeys)
}

func (c *TrpcConfig) search(unmarshedData map[string]interface{}, subkeys []string) (interface{}, error) {
	if len(subkeys) == 0 {
		return nil, ErrConfigNotExist
	}

	next, ok := unmarshedData[subkeys[0]]
	if ok {
		if len(subkeys) == 1 {
			return next, nil
		}

		switch next.(type) {

		case map[interface{}]interface{}:
			return c.search(cast.ToStringMap(next), subkeys[1:])

		case map[string]interface{}:
			return c.search(next.(map[string]interface{}), subkeys[1:])

		default:
			return nil, ErrConfigNotExist
		}
	}
	return nil, ErrConfigNotExist
}

// GetString 根据key读取string类型配置
func (c *TrpcConfig) GetString(key string, defaultValue string) string {
	subkeys := c.parseKey(key)

	value, err := c.locateSubkey(subkeys)
	if err != nil {
		return defaultValue
	}

	if result, ok := value.(string); ok {
		return result
	}

	if result, err := cast.ToStringE(value); err == nil {
		return result
	}

	return defaultValue
}

// Load 加载配置
func (c *TrpcConfig) Load() error {
	if c.p == nil {
		return ErrProviderNotExist
	}

	data, err := c.p.Read(c.path)
	if err != nil {
		return fmt.Errorf("trpc/config: failed to load %s: %s", c.path, err.Error())
	}
	c.rawData = data
	c.unmarshedData = map[string]interface{}{}
	err = c.decoder.Unmarshal(c.rawData, &c.unmarshedData)
	if err != nil {
		return fmt.Errorf("trpc/config: failed to parse %s: %s", c.path, err.Error())
	}
	return nil
}

// Reload 重新载入
func (c *TrpcConfig) Reload() {
	if c.p == nil {
		return
	}

	data, err := c.p.Read(c.path)
	if err != nil {
		log.Tracef("trpc/config: failed to reload %s: %v", c.path, err)
		return
	}

	unmarshedData := map[string]interface{}{}
	if err = c.decoder.Unmarshal(data, &unmarshedData); err != nil {
		log.Tracef("trpc/config: failed to parse %s: %v", c.path, err)
		return
	}

	c.rawData = data
	c.unmarshedData = unmarshedData
}

// Unmarshal 反序列化
func (c *TrpcConfig) Unmarshal(out interface{}) error {
	return c.decoder.Unmarshal(c.rawData, out)
}

func (c *TrpcConfig) parseKey(key string) []string {
	return strings.Split(key, ".")
}

func newTrpcConfig(path string) *TrpcConfig {
	yc := &TrpcConfig{
		p:       GetProvider("file"),
		path:    path,
		decoder: &YamlCodec{},
	}
	return yc
}
