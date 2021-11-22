# 为 tRPC-Go 作出贡献

欢迎您 [提出问题](issues) 或 [merge requests](merge_requests)， 建议您在为 tRPC-Go 作出贡献前先阅读以下 tRPC-Go 贡献指南。

### 代码规范
 
必须遵循[腾讯Golang代码规范](https://git.code.oa.com/standards/go)

### Commit Mesage 编写规范

每次提交，Commit message 都包括三个部分：Header，Body 和 Footer。
 
```html
<type>(<scope>): <subject>
// 空一行
<body>
// 空一行
<footer>
```
 
Header部分只有一行，包括三个字段：type（必需）、scope（可选）和subject（必需）。
 
type 用于说明 commit 的类别，只允许使用下面7个标识。
 
- feat：新功能（feature）
- fix：修补bug
- docs：文档（documentation）
- style： 格式（不影响代码运行的变动）
- refactor：重构（即不是新增功能，也不是修改bug的代码变动）
- test：增加测试
- chore：构建过程或辅助工具的变动
 
scope 用于指定 commit 影响的范围，比如数据层、控制层、视图层等等，或者具体所属package。
 
subject是 commit 目的的简短描述。
 
Body 部分是对本次 commit 的详细描述。
 
Footer 部分关闭 Issue。
 
如果当前 commit 针对某个issue，那么可以在 Footer 部分关闭这个 issue 。
 
```html
feat(transport): add transport stream support
 
- support only client stream 
- support only server stream 
- supoort both client and server stream
 
Close #1
```

### 分支管理

tRPC-Go 主仓库一共包含一个master分支和多个release分支:

release 分支

请勿在 release 分支上提交任何 MR。

master 分支

master 分支作为稳定的开发分支，经过测试后会在下一个版本合并到 release 分支。
MR 的目标分支应该是 master 分支。

```html
trpc-go/trpc-go/r0.1
 ↑ 经过测试之后合并主干，发布版本
trpc-go/trpc-go/master
 ↑ 开发者提出MR，合并进入主仓库开发分支
your_repo/trpc-go/master
```

### MR流程规范

对于所有的 MR，我们会运行一些代码检查和测试，一经测试通过，会接受这次 MR，但不会立即将代码合并到 release 分支上，会有一些延迟。

当您准备 MR 时，请确保已经完成以下几个步骤:

1. 将主仓库代码 Fork 到自己名下。
2. 基于 master 分支创建您的开发分支。
3. 检查您的代码语法及格式，确保符合腾讯Golang规范。
4. 提一个 MR 到主仓库的 master 分支上，指定相应模块的owner为reviewer。
