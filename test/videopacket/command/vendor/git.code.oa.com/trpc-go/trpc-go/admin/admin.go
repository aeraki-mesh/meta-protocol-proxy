// Package admin 实现了一些常用的管理功能
package admin

import (
	"errors"
	"io/ioutil"
	"net/http"
	"net/http/pprof"
	"sync"

	"git.code.oa.com/trpc-go/trpc-go/config"
	"git.code.oa.com/trpc-go/trpc-go/log"

	jsoniter "github.com/json-iterator/go"
)

// assert trpcAdminServer implements AdminServer interface
var defaultAdminServer Server

// var defaultRouter Router
var defaultRouter = NewRouter()

var (
	pattenCmds     = "/cmds"
	pattenVersion  = "/version"
	pattenLoglevel = "/cmds/loglevel"
	pattenConfig   = "/cmds/config"

	json = jsoniter.ConfigCompatibleWithStandardLibrary
)

// return param
var (
	ReturnErrCodeParam = "errorcode"
	ReturnMessageParam = "message"
	ErrCodeServer      = 1
)

// 初始化
func init() {
	a := &trpcAdminServer{
		router: defaultRouter,
	}

	a.router.Config(pattenCmds, a.handleCmds).Desc("管理命令列表")
	a.router.Config(pattenVersion, a.handleVersion).Desc("框架版本")
	a.router.Config(pattenLoglevel, a.handleLogLevel).Desc("查看/设置框架的日志级别")
	a.router.Config(pattenConfig, a.handleConfig).Desc("查看框架配置文件")

	a.router.Config("/debug/pprof/", pprof.Index)
	a.router.Config("/debug/pprof/cmdline", pprof.Cmdline)
	a.router.Config("/debug/pprof/profile", pprof.Profile)
	a.router.Config("/debug/pprof/symbol", pprof.Symbol)
	a.router.Config("/debug/pprof/trace", pprof.Trace)

	defaultAdminServer = a
}

// Server 是管理服务的接口, 提供启动、关闭、获取/自定义路由的基础功能
type Server interface {
	// 启动admin服务
	Run(opts ...Option) error

	// 关闭admin服务
	Close() error
}

// Run 方法提供启动admin服务的功能
func Run(opts ...Option) error {
	return defaultAdminServer.Run(opts...)
}

// Close 关闭admin服务
func Close() error {
	return defaultAdminServer.Close()
}

// HandleFunc 注册自定义服务接口
func HandleFunc(patten string, handler func(w http.ResponseWriter, r *http.Request)) *RouterHandler {
	return defaultRouter.Config(patten, handler)
}

// SetAdminServer 插件化替换admin服务
func SetAdminServer(a Server) {
	if a != nil {
		defaultAdminServer = a
	}
}

type trpcAdminServer struct {
	config    adminConfig
	server    *http.Server
	closeOnce sync.Once
	closeErr  error
	router    Router
}

// Run 启动admin服务，加载配置，接收外部请求
func (a *trpcAdminServer) Run(opts ...Option) error {
	a.config = loadDefaultConfig()
	for _, opt := range opts {
		opt(&a.config)
	}

	if a.config.enableTLS {
		return errors.New("not support yet")
	}

	a.server = &http.Server{
		Addr:         a.config.getAddr(),
		ReadTimeout:  a.config.readTimeout,
		WriteTimeout: a.config.writeTimeout,
		Handler:      a.router,
	}

	return a.server.ListenAndServe()
}

// Close 关闭服务
func (a *trpcAdminServer) Close() error {
	a.closeOnce.Do(a.close)
	return a.closeErr
}

func (a *trpcAdminServer) close() {
	if a.server == nil {
		return
	}
	a.closeErr = a.server.Close()
}

// ErrorOutput 统一错误输出
func ErrorOutput(w http.ResponseWriter, error string, code int) {
	var ret = getDefaultRes()
	ret[ReturnErrCodeParam] = code
	ret[ReturnMessageParam] = error
	_ = json.NewEncoder(w).Encode(ret)
}

// handleCmds 管理命令列表
func (a *trpcAdminServer) handleCmds(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.Header().Set("Content-Type", "application/json; charset=utf-8")

	var cmds []string
	list := a.router.List()
	for _, item := range list {
		cmds = append(cmds, item.GetPatten())
	}
	var ret = getDefaultRes()
	ret["cmds"] = cmds

	_ = json.NewEncoder(w).Encode(ret)
}

// getDefaultRes admin默认输出格式
func getDefaultRes() map[string]interface{} {
	return map[string]interface{}{
		ReturnErrCodeParam: 0,
		ReturnMessageParam: "",
	}
}

// handleVersion 版本号查询
func (a *trpcAdminServer) handleVersion(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.Header().Set("Content-Type", "application/json; charset=utf-8")

	ret := map[string]interface{}{
		ReturnErrCodeParam: 0,
		ReturnMessageParam: "",
		"version":          a.config.version,
	}
	_ = json.NewEncoder(w).Encode(ret)
}

// getLevel 获取logger对象output流等级
func getLevel(logger log.Logger, output string) string {
	level := logger.GetLevel(output)
	return log.LevelStrings[level]
}

// handleLogLevel 查询设置日志等级
func (a *trpcAdminServer) handleLogLevel(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.Header().Set("Content-Type", "application/json; charset=utf-8")

	if err := r.ParseForm(); err != nil {
		ErrorOutput(w, err.Error(), ErrCodeServer)
		return
	}

	name := r.Form.Get("logger")
	output := r.Form.Get("output")

	if name == "" {
		name = "default"
	}
	if output == "" {
		output = "0" // 没有ouput，默认为第一个output，一般用户也只配置一个
	}

	logger := log.Get(name)
	if logger == nil {
		ErrorOutput(w, "logger not found", ErrCodeServer)
		return
	}

	var ret = getDefaultRes()
	if r.Method == http.MethodGet {
		ret["level"] = getLevel(logger, output)
		_ = json.NewEncoder(w).Encode(ret)

	} else if r.Method == http.MethodPut {
		level := r.PostForm.Get("value")

		ret["prelevel"] = getLevel(logger, output)
		logger.SetLevel(output, log.LevelNames[level])
		ret["level"] = getLevel(logger, output)

		_ = json.NewEncoder(w).Encode(ret)
	}
}

// handleConfig 配置文件内容查询
func (a *trpcAdminServer) handleConfig(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("X-Content-Type-Options", "nosniff")
	w.Header().Set("Content-Type", "application/json; charset=utf-8")

	buf, err := ioutil.ReadFile(a.config.configPath)
	if err != nil {
		ErrorOutput(w, err.Error(), ErrCodeServer)
		return
	}

	unmarshaler := config.GetUnmarshaler("yaml")
	if unmarshaler == nil {
		ErrorOutput(w, "cannot failed yaml unmarshaler", 1)
		return
	}

	conf := map[interface{}]interface{}{}
	if err = unmarshaler.Unmarshal(buf, &conf); err != nil {
		ErrorOutput(w, err.Error(), ErrCodeServer)
		return
	}

	var ret = getDefaultRes()
	ret["content"] = conf

	_ = json.NewEncoder(w).Encode(ret)
}
