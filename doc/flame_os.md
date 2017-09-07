
## `namespace flame\os`

封装系统相关的 API，目前提供了启动子进程等功能；

#### `flame\os\process flame\os\start_process(string $exec[, array $argv[, array $env[, string $cwd[, array $opts])`
启动进程，相关参数如下：
* `$exec` - 可执行文件名或路径；
* `$argv` - array 参数数组 null 无参数；
* `$env`  - array 环境变量 null 继承父进程当前环境变量；
* `$cwd`  - string 子进程工作路径 null 使用父进程当前工作路径；
* `$opts` - 其他选项，存在如下选项：
	* `gid` / `uid` - 设置启动进程的组、用户ID（需要 root 权限）；
	* `detach` - 脱离父子进程关系；
	* `stdout` / `stderr` - 重定向子进程的输出到指定文件路径；

**示例**：
``` PHP
<?php
$proc = flame\os\start_process("ping", ["www.baidu.com"], ["ENV_KEY_1"=>"ENV_VAL_1"], "/tmp", [
	"gid" => 2017, "uid" => 2017,
	"stdout" => "/tmp/ping.log",
]);
```

**注意**：
* 额外选项中 `detach` 与 `stdout` | `stderr` 不能同时使用；

#### `class flame\os\process`
进程对象

##### `function process::kill([$signal = SIGTERM])`
向当前进程发送信号，默认 SIGTERM 信号

##### `yield function process::wait()`
等待当前进程结束
