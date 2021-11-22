// Package server 服务端，包括网络通信 名字服务 监控统计 链路跟踪等各个组件基础接口，具体实现由第三方middleware注册进来
package server

import (
	"context"
	"errors"
	"sync"
	"time"
)

// Server trpc server 一个服务进程只有一个server，一个server可以有多个service
type Server struct {
	services map[string]Service // k=serviceName,v=Service
}

// AddService 添加一个service到server里面，service name为配置文件指定的用于名字服务的name
// trpc.NewServer() 会遍历配置文件中定义的service配置项，调用AddService完成serviceName与实现的映射关系
func (s *Server) AddService(serviceName string, service Service) {

	if s.services == nil {
		s.services = make(map[string]Service)
	}
	s.services[serviceName] = service
}

// Service 通过serviceName获取对应的Service
func (s *Server) Service(serviceName string) Service {

	if s.services == nil {
		return nil
	}
	return s.services[serviceName]
}

// Register 把业务实现接口注册到server里面，一般一个server只有一个service，
// 有多个service的情况下请使用 Service("servicename") 指定, 否则默认会把实现注册到server里面的所有service
func (s *Server) Register(serviceDesc interface{}, serviceImpl interface{}) error {

	desc, ok := serviceDesc.(*ServiceDesc)
	if !ok {
		return errors.New("service desc type invalid")
	}

	for _, srv := range s.services {
		err := srv.Register(desc, serviceImpl)
		if err != nil {
			return err
		}
	}

	return nil
}

// Close 通知各个service执行关闭动作,最长等待10s
func (s *Server) Close(ch chan struct{}) error {

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()

	var wg sync.WaitGroup
	for _, service := range s.services {

		wg.Add(1)
		go func(srv Service) {

			defer wg.Done()

			c := make(chan struct{}, 1)
			go srv.Close(c)

			select {
			case <-c:
			case <-ctx.Done():
			}
		}(service)
	}

	wg.Wait()
	if ch != nil {
		ch <- struct{}{}
	}

	return nil
}
