# tRPC-Go配置库

trpc-go/config是一个组件化的配置库，提供了一种简单方式读取多种内容源、多种文件类型的配置。

## 功能

- 插件化：根据配置需要可从多个内容源（本地文件、TConf等）加载配置。

- 热加载：变更时自动载入新配置

- 默认设置：配置由于某些原因丢失时，可以回退使用默认值。

## 相关结构

- Config： 配置的通用接口，提供了相关的配置读取操作。

- ConfigLoader： 配置加载器，通过实现ConfigLoader相关接口以支持加载策略。

- Codec： 配置编解码接口，通过实现Codec相关接口以支持多种类型配置。

- DataProvider: 内容源接口，通过实现DataProvider相关接口以支持多种内容源。目前支持`file`，`tconf`，`rainbow`等配置内容源。

## 如何使用

```go
import (
    "git.code.oa.com/trpc-go/trpc-go/config"
    _ "git.code.oa.com/trpc-go/trpc-config-tconf" // 使用TConf配置时引入注册
    _ "git.code.oa.com/trpc-go/trpc-config-rainbow" // 使用rainbow配置时引入注册
)


// 加载本地配置文件: config.WithProvider("file")
config.Load("../testdata/trpc_go.yaml", config.WithCodec("yaml"), config.WithProvider("file"))

// 默认的DataProvider是使用本地文件
config.Load("../testdata/trpc_go.yaml", config.WithCodec("yaml"))

// 加载TConf配置文件: config.WithProvider("tconf")
c, _ := config.Load("test.yaml", config.WithCodec("yaml"), config.WithProvider("tconf"))

// 读取bool类型配置
c.GetBool("server.debug", false)

// 读取String类型配置
c.GetString("server.app", "default")

```

### 并发安全的监听远程配置变化

```go
import (
	"sync/atomic"
    ...
)

type yamlFile struct {
    Server struct {
        App string
    }
}

// 参考：https://golang.org/pkg/sync/atomic/#Value
var cfg atomic.Value // 并发安全的Value

// 使用trpc-go/config中Watch接口监听tconf远程配置变化
c, _ := config.Get("tconf").Watch(context.TODO(), "test.yaml")

go func() {
    for r := range c {
        yf := &yamlFile{}
        fmt.Printf("event: %d, value: %s", r.Event(), r.Value())

        if err := yaml.Unmarshal([]byte(r.Value()), yf); err == nil {
            cfg.Store(yf)
        }

    }
}()

// 当配置初始化完成后，可以通过 atomic.Value 的 Load 方法获得最新的配置对象
cfg.Load().(*yamlFile)

```

### 如何mock Watch
业务代码在单元测试时，需要对相关函数进行打桩

其他的方法需要mock，也是使用相同的方法，先打桩替换你需要的实现
```go

import (
    "context"
    "fmt"
    "reflect"
    "testing"

    tconf "git.code.oa.com/trpc-go/trpc-config-tconf"
    "git.code.oa.com/trpc-go/trpc-go/config"

    "github.com/agiledragon/gomonkey"
    "github.com/stretchr/testify/assert"
)

func TestWatch(t *testing.T) {

    // mock
    mockStr := "config-test"
    mockResp := &mockResponse{val: mockStr}
    mockChan := make(chan config.Response, 1)

    // 相关方法打桩
    gomonkey.ApplyFunc(config.Get, func(_ string) config.KVConfig {
        return (*tconf.Client)(nil)
    })
    gomonkey.ApplyMethod(reflect.TypeOf(config.Get("tconf")), "Watch", func(_ *tconf.Client, ctx context.Context, key string, opts ...confi    g.Option) (<-chan config.Response, error) {
        return mockChan, nil
    })
    mockChan <- mockResp


    // action
    respChan, _ := config.Get("tconf").Watch(context.TODO(), "")
    resp := <-respChan

    // assert
    assert.Equal(t, resp.Value(), mockStr)

}

type mockResponse struct {
    val string
}

func (r *mockResponse) Value() string {
    return r.val
}

func (r *mockResponse) MetaData() map[string]string {
    return nil
}

func (r *mockResponse) Event() config.EventType {
    return config.EventTypeNull
}


```

