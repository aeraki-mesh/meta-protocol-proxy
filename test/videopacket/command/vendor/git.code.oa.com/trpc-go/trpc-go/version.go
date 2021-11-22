package trpc

import "fmt"

// trpc版本号规则
// 1. MAJOR version when you make incompatible API changes；
// 2. MINOR version when you add functionality in a backwards-compatible manner；
// 3. PATCH version when you make backwards-compatible bug fixes；
// 4. Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format；
// 内测版        0.1.0-alpha
// 公测版        0.1.0-beta
// release后选版 0.1.0-rc
// 正式release版 0.1.0
const (
	MajorVersion  = 0
	MinorVersion  = 3
	PatchVersion  = 5
	VersionSuffix = "" // -alpha -alpha.1 -beta -rc -rc.1
)

// Version 返回trpc框架的版本号
func Version() string {
	return fmt.Sprintf("v%d.%d.%d%s", MajorVersion, MinorVersion, PatchVersion, VersionSuffix)
}
