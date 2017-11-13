
## `namespace flame\os`

封装系统相关的 API，目前提供了启动子进程等功能；

#### `flame\os\process flame\os\spawn(string $exec[, array $argv[, array $env[, string $cwd[, array $opts])`
启动进程，相关参数如下：
* `$exec` - 可执行文件名或路径；
* `$argv` - array 参数数组 null 无参数；
* `$env`  - array 环境变量 null 继承父进程当前环境变量；
* `$cwd`  - string 子进程工作路径 null 使用父进程当前工作路径；
* `$opts` - 其他选项，存在如下选项：
	* `gid` / `uid` - 用于切换进程用户，指定组、用户ID（需要 root 权限）；
	* `detach` - 脱离父子进程关系；
	* `stdout` / `stderr` - 重定向子进程的输出到指定文件路径；
	* `ipc` - 建立 IPC 通讯管道（能够传输 文本 及 套接字）；
函数成功，则返回启动的进程对应的 `process` 对象；否则抛出异常；

**示例**：
``` PHP
<?php
$proc = flame\os\spawn("ping", ["www.baidu.com"], ["ENV_KEY_1"=>"ENV_VAL_1"], "/tmp", [
	"gid" => 2017, "uid" => 2017,
	"stdout" => "/tmp/ping.log",
]);
// 上述过程也可以直接使用 process 类型的构造函数
$proc = new flame\os\process("ping", ...);
```

**注意**：
* 请不要同时使用额外选项中 `detach` 与 `stdout` / `stderr` ；
* 若进程未与父进程脱离（`detach`），则当该进程对象销毁时，实际进程将被强制结束（SIGKILL）；
* 被启动的进程将会“异步”运行，不会阻塞当前 PHP 程序进程；

#### `class flame\os\process`
进程对象;

**注意**：
* 若启动进程时未指定 `detach` 分离父子进程，进程对象销毁将导致实际进程被强制结束；

##### `process::__construct(string $exec[, array $argv[, array $env[, string $cwd[, array $opts])`
与上述 `spawn()` 函数功能相同，请参考该函数相关说明；

##### `integer process::$pid`
被启动的进程的进程ID；

##### `function process::kill([$signal = SIGTERM])`
向当前进程发送信号，默认 SIGTERM 信号

##### `yield process::wait()`
等待当前进程结束

**注意**：
* 只能够在一个协程中调用 `yield $proc->wait()` 否则会导致协程僵死；

##### `yield process::send(mixed $data)`
向当前进程发送消息；`$data` 可以是文本消息或 `tcp_socket` / `unix_socket` 的套接字对象；

**注意**：
* 仅能够对 libuv 框架启用了 ipc 通讯管道的子进程使用；
* 文本消息长度不能超过 65535 字节，支持二进制数据；

#### `flame\os\process flame\os\cluster\fork()`
与 `flame\os\spawn()` 函数类似，但不需要传递任何参数；按默认方式启动子进程，并建立 IPC 通道。

#### `flame\os\cluster\send(string $data)`
向父进程发送消息，但仅允许文本消息（允许二进制文本数据）；

**注意**：
* 仅在被 flame 启动并启用了 ipc 通讯管道的子进程中可用；
* 文本消息长度不能超过 65535 字节，支持二进制数据；

#### `flame\os\cluster\handle(callable $cb)`
设置套接字传输处理器，接收来自父进程的套接字传输：回调函数 或 `flame\\net\\fastcgi\\handler` 对象等；

#### `flame\os\cluster\ondata(callable $cb)`
设置文本消息处理器，接收来自父进程或子进程的文本消息：回调函数；

#### `string flame\os\executable()`
返回当前运行的 PHP 进程路径。

**示例**：
``` PHP
<?php
$php_path = flame\os\executable();
// "/usr/local/php/bin/php"
```
