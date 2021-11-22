// +build !windows

package server

import (
	"context"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"time"

	"git.code.oa.com/trpc-go/trpc-go/log"
)

// EnvGraceRestartStr 是否热重启环境变量
const EnvGraceRestartStr = "TRPC_IS_GRACEFUL=1"

// Serve 启动所有服务
func (s *Server) Serve() (_ error) {

	if len(s.services) == 0 {
		panic("service empty")
	}

	ch := make(chan os.Signal)
	var failedServices sync.Map
	var err error
	for name, service := range s.services {

		go func(n string, srv Service) {

			e := srv.Serve()
			if e != nil {
				err = e
				failedServices.Store(n, srv)
				time.Sleep(time.Millisecond * 300)
				ch <- syscall.SIGTERM
			}
		}(name, service)
	}

	signal.Notify(ch, syscall.SIGINT, syscall.SIGTERM, syscall.SIGUSR2, syscall.SIGSEGV)

	sig := <-ch
	// 热重启单独处理
	if sig == syscall.SIGUSR2 {
		_, err = s.StartNewProcess()
		if err != nil {
			panic(err)
		}
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Second*10)
	defer cancel()

	var wg sync.WaitGroup
	for name, service := range s.services {

		if _, ok := failedServices.Load(name); ok {
			continue
		}

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

	if err != nil {
		panic(err)
	}

	return nil
}

// StartNewProcess 启动新进程, 由于trpc使用了reuseport形式，可以直接fork子进程
// 不必再传递net.Listener的文件描述符
func (s *Server) StartNewProcess() (uintptr, error) {

	log.Infof("receive USR2 signal, so restart the process")

	execSpec := &syscall.ProcAttr{
		Env:   append(os.Environ(), EnvGraceRestartStr),
		Files: []uintptr{os.Stdin.Fd(), os.Stdout.Fd(), os.Stderr.Fd()},
	}
	fork, err := syscall.ForkExec(os.Args[0], os.Args, execSpec)
	if err != nil {
		log.Error("failed to forkexec with err: %s", err.Error())
		return 0, err
	}

	return uintptr(fork), nil
}
