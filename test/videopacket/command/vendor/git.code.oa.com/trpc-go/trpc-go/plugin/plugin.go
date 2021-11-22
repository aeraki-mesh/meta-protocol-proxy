// Package plugin 通用插件工厂体系，提供插件注册和装载, 主要用于需要通过动态配置加载生成具体插件的情景, 不需要配置生成的插件 可直接注册到具体的插件包里面(如codec) 不需要注册到这里
package plugin

import (
	"errors"
	"fmt"
	"sync"
	"time"

	yaml "gopkg.in/yaml.v3"
)

var (
	// SetupTimeout 每个插件初始化最长超时时间
	SetupTimeout = time.Second * 3

	// MaxPluginSize  最大插件个数
	MaxPluginSize = 1000

	// plugin type => { plugin name => plugin factory }
	plugins = make(map[string]map[string]Factory)

	lock = sync.RWMutex{}
	done = make(chan struct{}, 1)
)

// WaitForDone 挂住等待所有插件初始化完成，可自己设置超时时间
func WaitForDone(timeout time.Duration) bool {
	select {
	case <-done:
		done <- struct{}{} // 每调用一次取出一个struct，再放回一个struct，允许多次调用判断是否已经初始化完成
		return true
	case <-time.After(timeout):
	}

	return false
}

// Factory 插件工厂统一抽象 外部插件需要实现该接口, 通过该工厂接口生成具体的插件并注册到具体的插件类型里面
type Factory interface {
	// Type 插件的类型 如 selector log config tracing
	Type() string
	// Setup 根据配置项节点装载插件, 用户自己先定义好具体插件的配置数据结构
	Setup(name string, configDec Decoder) error
}

// Decoder 节点配置解析器
type Decoder interface {
	Decode(conf interface{}) error
}

// YamlNodeDecoder  yaml.Node decoder
type YamlNodeDecoder struct {
	Node *yaml.Node
}

// Decode 解析yaml node配置
func (d *YamlNodeDecoder) Decode(conf interface{}) error {

	if d.Node == nil {
		return errors.New("yaml node empty")
	}
	return d.Node.Decode(conf)
}

// Register 注册插件工厂 可自己指定插件名，支持相同的实现 不同的配置注册不同的工厂实例
func Register(name string, p Factory) {
	lock.Lock()
	defer lock.Unlock()

	pluginFactories, ok := plugins[p.Type()]

	if !ok {
		plugins[p.Type()] = map[string]Factory{
			name: p,
		}
		return
	}

	pluginFactories[name] = p
}

// Get 根据插件类型，插件名字获取插件工厂
func Get(pluginType string, name string) Factory {

	lock.RLock()
	pluginFactories, ok := plugins[pluginType]

	if !ok {
		lock.RUnlock()
		return nil
	}

	f := pluginFactories[name]
	lock.RUnlock()
	return f
}

// Config 插件统一配置 plugin type => { plugin name => plugin config }
type Config map[string]map[string]yaml.Node

// Depender 依赖接口，由具体实现插件决定是否有依赖插件, 需要保证被依赖的插件先初始化完成
type Depender interface {
	DependsOn() []string // 假如一个插件依赖另一个插件，则返回被依赖的插件的列表：数组元素为 type-name 如 [ "selector-polaris" ]
}

// Setup 通过配置生成并装载具体插件
func (p Config) Setup() (err error) {

	pluginChan := make(chan Info, MaxPluginSize)
	setupPlugins := make(map[string]bool)

	for pluginType, pluginFactories := range p {
		for pluginName, pluginConfig := range pluginFactories {

			factory := Get(pluginType, pluginName)
			if factory == nil {
				fmt.Printf("plugin %s:%s no registered!\n", pluginType, pluginName)
				continue
			}

			p := Info{
				plugin: factory,
				typ:    pluginType,
				name:   pluginName,
				conf:   pluginConfig,
			}
			select {
			case pluginChan <- p:
			default:
				return fmt.Errorf("plugin number exceed max limit:%d", MaxPluginSize)
			}

			setupPlugins[p.Key()] = false
		}
	}

	num := len(pluginChan)
	for num > 0 {
		for i := 0; i < num; i++ {

			p := <-pluginChan

			deps, err := p.Depends(setupPlugins)
			if err != nil {
				return err
			}
			if deps { // 被依赖的插件还未初始化，将当前插件移到channel末尾
				pluginChan <- p
				continue
			}

			err = p.Setup()
			if err != nil {
				return err
			}
			setupPlugins[p.Key()] = true
		}

		if len(pluginChan) == num { // 循环依赖导致无插件可以初始化，返回失败
			return fmt.Errorf("cycle depends, not plugin is setup")
		}

		num = len(pluginChan)
	}

	select {
	case done <- struct{}{}:
	default:
	}

	return nil
}

// Info 插件信息
type Info struct {
	plugin Factory
	typ    string
	name   string
	conf   yaml.Node
}

// Depends 判断是否有依赖的插件未初始化过
func (p *Info) Depends(setupPlugins map[string]bool) (bool, error) {

	deps, ok := p.plugin.(Depender)
	if !ok { // 该插件不依赖任何其他插件
		return false, nil
	}

	depends := deps.DependsOn()
	for _, name := range depends {

		if name == p.Key() {
			return false, fmt.Errorf("cannot depends on self")
		}

		setup, ok := setupPlugins[name]
		if !ok {
			return false, fmt.Errorf("depends plugin %s not exists", name)
		}

		if !setup {
			return true, nil
		}
	}

	return false, nil
}

// Setup 初始化单个插件
func (p *Info) Setup() (err error) {

	ch := make(chan struct{}, 1)
	go func() {
		err = p.plugin.Setup(p.name, &YamlNodeDecoder{Node: &p.conf})
		ch <- struct{}{}
	}()

	select {
	case <-ch:
	case <-time.After(SetupTimeout):
		return fmt.Errorf("setup plugin %s timeout", p.Key())
	}

	if err != nil {
		return err
	}

	return nil
}

// Key 插件的唯一索引
func (p *Info) Key() string {

	return fmt.Sprintf("%s-%s", p.typ, p.name)
}
