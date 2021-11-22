package config

import (
	"io/ioutil"
	"path/filepath"

	"git.code.oa.com/trpc-go/trpc-go/log"

	"github.com/fsnotify/fsnotify"
)

func init() {
	RegisterProvider(newFileProvider())
}

func newFileProvider() *FileProvider {
	fp := &FileProvider{
		cache:           make(map[string]string),
		cb:              make(chan ProviderCallback),
		disabledWatcher: true,
	}
	if watcher, err := fsnotify.NewWatcher(); err == nil {
		fp.disabledWatcher = false
		fp.watcher = watcher
		go fp.run()
	}

	return fp
}

// FileProvider 从文件系统拉取文件内容
type FileProvider struct {
	disabledWatcher bool
	watcher         *fsnotify.Watcher
	cb              chan ProviderCallback
	cache           map[string]string
}

// Name Provider名字
func (*FileProvider) Name() string {
	return "file"
}

// Read 读取指定文件
func (fp *FileProvider) Read(path string) ([]byte, error) {

	if !fp.disabledWatcher {
		if err := fp.watcher.Add(path); err != nil {
			return nil, err
		}
		fp.cache[filepath.Clean(path)] = path
	}

	data, err := ioutil.ReadFile(path)
	if err != nil {
		log.Tracef("Failed to read file %v", err)
		return nil, err
	}
	return data, nil
}

// Watch 注册文件变化处理函数
func (fp *FileProvider) Watch(cb ProviderCallback) {
	if !fp.disabledWatcher {
		fp.cb <- cb
	}
}

func (fp *FileProvider) run() {

	fn := make([]ProviderCallback, 0)

	for {
		select {

		case i := <-fp.cb:
			fn = append(fn, i)

		case e := <-fp.watcher.Events:
			if data, err := ioutil.ReadFile(e.Name); err == nil {
				if path, ok := fp.cache[e.Name]; ok {
					for _, f := range fn {
						go f(path, data)
					}
				}
			}
		}

	}
}
