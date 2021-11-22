# tRPC-Go 拦截器功能及实现 

框架在server业务处理函数handler前后，和client网络调用函数call前后，分别预埋了四个点允许用户自定义任何的逻辑来拦截处理。

## 自定义一个拦截器

### 定义处理逻辑函数

```golang
func ServerFilter() filter.Filter {
    return func(ctx context.Context, req, rsp interface{}, handler filter.HandleFunc) (err error) {
        
        //前置逻辑
        
        err = handler(ctx, req, rsp)
        
        //后置逻辑
        
        return err //返回error给上层
    }
}
```
```golang
func ClientFilter() filter.Filter {
    return func(ctx context.Context, req, rsp interface{}, handler filter.HandleFunc) (err error) {
        
        //前置逻辑
        
        err = handler(ctx, req, rsp)
        
        //后置逻辑
        
        return err //返回error给上层
    }
}
```

### 注册到框架中
```golang
filter1 := ServerFilter()
filter2 := ClientFilter()

filter.Register("name", filter1, filter2)
```

### 配置文件开启使用
```yaml
server:
 ...
 filter:
  ...
  - name 

client:
 ...
 filter:
  ...
  - name 
```