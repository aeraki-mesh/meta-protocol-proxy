package admin

import (
	"fmt"
	"net/http"
	"sync"
)

// Router 路由表接口，通过实现该接口的结构注册路由信息
type Router interface {
	// 设置处理函数，不可覆盖
	Config(patten string, handler func(w http.ResponseWriter, r *http.Request)) *RouterHandler

	// ServeHTTP dispatches the request to the handler whose pattern most closely matches the request URL.
	ServeHTTP(w http.ResponseWriter, req *http.Request)

	// List 列出目前的注册方法
	List() []*RouterHandler
}

// NewRouter 创建一个新的Router
func NewRouter() Router {
	return &router{
		ServeMux: http.NewServeMux(),
	}
}

// NewRouterHandler 创建一个新的restful路由信息处理者
func NewRouterHandler(patten string, handler func(w http.ResponseWriter, r *http.Request)) *RouterHandler {
	return &RouterHandler{
		patten:  patten,
		handler: handler,
	}
}

type router struct {
	*http.ServeMux

	sync.RWMutex
	handleFuncMap map[string]*RouterHandler
}

// Config 用来配置一个路由的pattern及处理函数
func (r *router) Config(patten string, handler func(w http.ResponseWriter, r *http.Request)) *RouterHandler {
	r.Lock()
	defer r.Unlock()

	r.ServeMux.HandleFunc(patten, handler)
	if r.handleFuncMap == nil {
		r.handleFuncMap = make(map[string]*RouterHandler)
	}

	handle := NewRouterHandler(patten, handler)
	r.handleFuncMap[patten] = handle

	return handle
}

// ServeHTTP 对传入的http请求进行处理
func (r *router) ServeHTTP(w http.ResponseWriter, req *http.Request) {
	defer func() {
		if err := recover(); err != nil {
			var ret = getDefaultRes()
			ret[ReturnErrCodeParam] = http.StatusInternalServerError
			ret[ReturnMessageParam] = fmt.Sprintf("PANIC : %v", err)
			_ = json.NewEncoder(w).Encode(ret)
		}
	}()

	r.ServeMux.ServeHTTP(w, req)
}

// List 返回配置的路由列表
func (r *router) List() []*RouterHandler {
	l := make([]*RouterHandler, 0, len(r.handleFuncMap))
	for _, handler := range r.handleFuncMap {
		l = append(l, handler)
	}
	return l
}

// RouterHandler 路由信息处理者
type RouterHandler struct {
	handler func(w http.ResponseWriter, r *http.Request)
	patten  string
	desc    string
}

// GetHandler 获取路由信息处理函数
func (r *RouterHandler) GetHandler() func(w http.ResponseWriter, r *http.Request) {
	return r.handler
}

// GetDesc 获取路由描述/备注
func (r *RouterHandler) GetDesc() string {
	return r.desc
}

// GetPatten 获取路由配置的模版
func (r *RouterHandler) GetPatten() string {
	return r.patten
}

// Desc 设置描述信息
func (r *RouterHandler) Desc(desc string) *RouterHandler {
	r.desc = desc
	return r
}
