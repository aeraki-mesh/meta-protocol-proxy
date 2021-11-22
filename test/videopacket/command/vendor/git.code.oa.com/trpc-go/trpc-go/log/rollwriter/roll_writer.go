// Package rollwriter 提供了一个高性能的文件滚动日志写入器
// rollwriter本身不提供打印日志功能，而是实现了io.Writer接口
// 因此该组件可以与任何可以写入io.Writer的日志包进行配合(包括标准库的log包)
// 主要特性：
// 1. 支持日志按文件大小进行滚动
// 2. 支持日志按时间进行滚动
// 3. 支持自动清理过期或者多余的日志文件
// 4. 支持压缩日志文件
package rollwriter

import (
	"compress/gzip"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"
	"time"

	"github.com/lestrrat-go/strftime"
)

const (
	backupTimeFormat = "bk-20060102-150405.000"
	compressSuffix   = ".gz"
	defaultMaxSize   = 100
)

// ensure we always implement io.WriteCloser
var _ io.WriteCloser = (*RollWriter)(nil)

// RollWriter 同时支持按大小和时间滚动的文件日志写入类
// 实现接口io.WriteCloser
type RollWriter struct {
	filePath string
	opts     *Options

	pattern  *strftime.Strftime
	currDir  string
	currPath string
	currSize int64
	currFile *os.File
	openTime int64

	mu       sync.Mutex
	once     sync.Once
	notifyCh chan bool
	closeCh  chan *os.File
}

// NewRollWriter 根据传入的参数创建一个RollWriter对象
func NewRollWriter(filePath string, opt ...Option) (*RollWriter, error) {
	opts := &Options{
		MaxSize:    defaultMaxSize * 1024 * 1024, //默认100M
		MaxAge:     0,                            //默认不清理旧文件
		MaxBackups: 0,                            //默认不删除多余文件
		Compress:   false,                        //默认不压缩
	}

	// 输入参数为最高优先级 覆盖掉原有数据
	for _, o := range opt {
		o(opts)
	}

	if filePath == "" {
		return nil, errors.New("invalid file path")
	}

	pattern, err := strftime.New(filePath + opts.TimeFormat)
	if err != nil {
		return nil, errors.New("invalid time pattern")
	}

	w := &RollWriter{
		filePath: filePath,
		opts:     opts,
		pattern:  pattern,
		currDir:  filepath.Dir(filePath),
	}

	err = os.MkdirAll(w.currDir, 0755)
	if err != nil {
		return nil, err
	}

	return w, nil
}

// Write 实现io.Writer
func (w *RollWriter) Write(v []byte) (n int, err error) {
	// 每隔10s重新打开一次文件
	if w.currFile == nil || time.Now().Unix()-w.openTime > 10 {
		w.mu.Lock()
		w.reopenFile()
		w.mu.Unlock()
	}

	// 文件创建失败，则返回
	if w.currFile == nil {
		return 0, errors.New("open file fail")
	}

	// 写日志到文件
	n, err = w.currFile.Write(v)
	w.currSize += int64(n)

	// 文件写满，开始滚动
	if w.currSize >= w.opts.MaxSize {
		w.mu.Lock()
		w.backupFile()
		w.mu.Unlock()
	}

	return n, err
}

// Close 关闭当前日志文件，实现io.Closer
func (w *RollWriter) Close() error {
	if w.currFile == nil {
		return nil
	}

	err := w.currFile.Close()
	w.currFile = nil
	return err
}

// reopenFile 定期重新打开文件
// 如果当前文件路径变更还需要通知清理协程
func (w *RollWriter) reopenFile() {
	if w.currFile == nil || time.Now().Unix()-w.openTime > 10 {
		w.openTime = time.Now().Unix()
		currPath := w.pattern.FormatString(time.Now())
		if w.currPath != currPath {
			w.currPath = currPath
			w.notify()
		}
		w.doReopenFile(w.currPath)
	}
}

// doReopenFile 重新打开文件
func (w *RollWriter) doReopenFile(path string) error {
	w.openTime = time.Now().Unix()
	lastFile := w.currFile

	of, err := os.OpenFile(path, os.O_WRONLY|os.O_APPEND|os.O_CREATE, 0666)
	if err == nil {
		w.currFile = of

		if lastFile != nil {
			// 有可能还在被使用，需要延迟关闭
			w.closeCh <- lastFile
		}

		st, _ := os.Stat(path)
		if st != nil {
			w.currSize = st.Size()
		}
	}

	return err
}

// backupFile 当文件超过大小限制，备份当前文件再重开一个新文件
func (w *RollWriter) backupFile() {
	if w.currSize >= w.opts.MaxSize {
		w.currSize = 0

		// 旧文件改名
		newName := w.currPath + "." + time.Now().Format(backupTimeFormat)
		if _, e := os.Stat(w.currPath); !os.IsNotExist(e) {
			os.Rename(w.currPath, newName)
		}

		// 开一个新文件
		w.doReopenFile(w.currPath)
		w.notify()
	}
}

// notify 通知清理协程执行清理动作
func (w *RollWriter) notify() {
	w.once.Do(func() {
		w.notifyCh = make(chan bool, 1)
		w.closeCh = make(chan *os.File, 100)
		go w.runCleanFiles()
		go w.runCloseFiles()
	})

	select {
	case w.notifyCh <- true:
	default:
	}
}

// runCloseFiles 启动一个协程
// 延迟关闭日志文件句柄
func (w *RollWriter) runCloseFiles() {
	for f := range w.closeCh {
		// 延迟20ms关闭
		time.Sleep(20 * time.Millisecond)
		f.Close()
	}
}

// runCleanFiles 启动一个协程
// 定期清理多余或者过期的日志文件/压缩日志文件
func (w *RollWriter) runCleanFiles() {
	for range w.notifyCh {
		if w.opts.MaxBackups == 0 && w.opts.MaxAge == 0 && !w.opts.Compress {
			continue
		}

		w.cleanFiles()
	}
}

// cleanFiles 清理多余或者过期的日志文件/压缩日志文件
func (w *RollWriter) cleanFiles() {
	// 获取和当前日志相关的日志文件列表
	files, err := w.getOldLogFiles()
	if err != nil || len(files) == 0 {
		return
	}

	// 按时间排序找出超过文件个数的待清理文件
	var compress, remove []logInfo
	files = filterByMaxBackups(files, &remove, w.opts.MaxBackups)

	// 根据文件最后修改时间找出过期的文件
	files = filterByMaxAge(files, &remove, w.opts.MaxAge)

	// 根据扩展名.gz，找出需要压缩的文件
	filterByCompressExt(files, &compress, w.opts.Compress)

	// 删除过期或者多余的日志文件
	w.removeFiles(remove)

	// 压缩需要压缩的日志文件
	w.compressFiles(compress)
}

// getOldLogFiles 返回和当前日志文件相关的日志文件列表（按修改时间排序）
func (w *RollWriter) getOldLogFiles() ([]logInfo, error) {
	files, err := ioutil.ReadDir(w.currDir)
	if err != nil {
		return nil, fmt.Errorf("can't read log file directory: %s", err)
	}

	logFiles := []logInfo{}
	filename := filepath.Base(w.filePath)
	for _, f := range files {
		if f.IsDir() {
			continue
		}

		if modTime, err := w.matchLogFile(f.Name(), filename); err == nil {
			logFiles = append(logFiles, logInfo{modTime, f})
		}
	}
	sort.Sort(byFormatTime(logFiles))

	return logFiles, nil
}

// matchLogFile 匹配和当前日志文件相关的所有日志文件，如果匹配则返回文件修改时间
func (w *RollWriter) matchLogFile(filename, filePrefix string) (time.Time, error) {
	// 排除当前日志文件
	// a.log
	// a.log.20200712
	if filepath.Base(w.currPath) == filename {
		return time.Time{}, errors.New("ignore current logfile")
	}

	// 匹配和当前日志文件相关的所有日志文件
	// a.log -> a.log.20200712-1232/a.log.20200712-1232.gz
	// a.log.20200712 -> a.log.20200712.20200712-1232/a.log.20200712.20200712-1232.gz
	if !strings.HasPrefix(filename, filePrefix) {
		return time.Time{}, errors.New("mismatched prefix")
	}

	if st, _ := os.Stat(filepath.Join(w.currDir, filename)); st != nil {
		return st.ModTime(), nil
	}

	return time.Time{}, errors.New("file stat fail")
}

// removeFiles 删除过期或者多余的日志文件
func (w *RollWriter) removeFiles(remove []logInfo) {
	// 清理文件（过期或者超过可保留文件个数）
	for _, f := range remove {
		os.Remove(filepath.Join(w.currDir, f.Name()))
	}
}

// compressFiles 压缩需要压缩的日志文件
func (w *RollWriter) compressFiles(compress []logInfo) {
	// 对日志文件进行压缩
	for _, f := range compress {
		fn := filepath.Join(w.currDir, f.Name())
		compressFile(fn, fn+compressSuffix)
	}
}

// filterByMaxBackups 过滤出超过限制多余的日志文件
func filterByMaxBackups(files []logInfo, remove *[]logInfo, maxBackups int) []logInfo {
	if maxBackups < len(files) {
		return files
	}

	var remaining []logInfo
	preserved := make(map[string]bool)
	for _, f := range files {
		fn := strings.TrimSuffix(f.Name(), compressSuffix)
		preserved[fn] = true

		if len(preserved) > maxBackups {
			*remove = append(*remove, f)
		} else {
			remaining = append(remaining, f)
		}
	}
	return remaining
}

// filterByMaxAge 过滤出过期的的日志文件
func filterByMaxAge(files []logInfo, remove *[]logInfo, maxAge int) []logInfo {
	if maxAge <= 0 {
		return files
	}

	var remaining []logInfo
	diff := time.Duration(int64(24*time.Hour) * int64(maxAge))
	cutoff := time.Now().Add(-1 * diff)
	for _, f := range files {
		if f.timestamp.Before(cutoff) {
			*remove = append(*remove, f)
		} else {
			remaining = append(remaining, f)
		}
	}

	return remaining
}

// filterByCompressExt 过滤出所有压缩文件
func filterByCompressExt(files []logInfo, compress *[]logInfo, needCompress bool) {
	if !needCompress {
		return
	}

	for _, f := range files {
		if !strings.HasSuffix(f.Name(), compressSuffix) {
			*compress = append(*compress, f)
		}
	}
}

// compressFile 压缩传入的文件，压缩成功则清理掉原始文件
func compressFile(src, dst string) (err error) {
	f, err := os.Open(src)
	if err != nil {
		return fmt.Errorf("failed to open file: %v", err)
	}
	defer f.Close()

	gzf, err := os.OpenFile(dst, os.O_CREATE|os.O_TRUNC|os.O_WRONLY, 0666)
	if err != nil {
		return fmt.Errorf("failed to open compressed file: %v", err)
	}
	defer gzf.Close()

	gz := gzip.NewWriter(gzf)
	defer func() {
		gz.Close()
		if err != nil {
			os.Remove(dst)
			err = fmt.Errorf("failed to compress file: %v", err)
		} else {
			os.Remove(src)
		}
	}()

	if _, err := io.Copy(gz, f); err != nil {
		return err
	}

	return nil
}

// logInfo 辅助结构体，用于返回文件名和最后修改时间
type logInfo struct {
	timestamp time.Time
	os.FileInfo
}

// byFormatTime 按时间降序排序
type byFormatTime []logInfo

// Less 判断b[j]的时间是否早于b[j]的时间
func (b byFormatTime) Less(i, j int) bool {
	return b[i].timestamp.After(b[j].timestamp)
}

// Swap 交换b[i]和b[j]
func (b byFormatTime) Swap(i, j int) {
	b[i], b[j] = b[j], b[i]
}

// Len 返回数组b的长度
func (b byFormatTime) Len() int {
	return len(b)
}
