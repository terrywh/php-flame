
## `namespace flame\os`

封装系统相关的 API，目前提供了启动子进程等功能；

#### `flame\os\process flame\os\spawn(string $exec[, array $argv[, array $opts])`
启动进程，相关参数如下：
* `$exec` - string - 可执行文件名或路径；
* `$argv` - array | null - 参数；
* `$opts` - array - 其他选项，存在如下选项：
	* `cwd` - string - 工作路径（目录）
	* `env` - map - 环境变量
	* `gid` - integer - 进程启动用户组；（须 ROOT 权限）
	* `uid` - integer - 进程启动用户；（须 ROOT 权限）
	* `detach` - boolean - 脱离父子进程关系；
	* `stdout` - string - 重定向子进程的标准输出到指定文件路径 或 管道：
	 	* "/tmp/ping.log" - 指定文件路径
		* "pipe" - 固定值用于读取输出数据（参见 `process::stdout()` 说明）；
	* `stderr` - string - 重定向进程的错误输出到指定的文件路径 或 管道：
		* "/tmp/ping.log" - 指定文件路径
		* "pipe" - 固定值用于读取输出数据（参见 `process::stderr()` 说明）；
	* `ipc` - boolean - 建立 IPC 通讯管道（使用 cluster 中的相关接口，传输 文本 及 套接字）；
	
函数成功，则返回启动的进程对应的 `process` 对象；否则抛出异常；

**示例**：
``` PHP
<?php
flame\go(function() {
	$proc = flame\os\spawn("ping", ["www.baidu.com"], [
		"env" => ["ENV_KEY_1"=>"ENV_VAL_1"],
		"cwd" => "/tmp",
		"gid" => 2017, "uid" => 2017,
		"stdout" => "/tmp/ping.log", // 重定向文件
		"stderr" => "pipe", // 重定向管道，通过 $proc->stderr() 获取 unix_socket 对象实例（只读）
	]);
	// 上述过程也可以直接使用 process 类型的构造函数
	$proc = new flame\os\process("ping", ...);
	// 读取错误输出
	$ss = $proc->stderr();
	while($data = yield $ss->read()) {
		var_dump($data);
	}
})
```

**注意**：
* 不能同时使用选项中 `detach` 与 `stdout` / `stderr` / `ipc` 提供的功能；
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

##### `process::stdout()`
获取进程的标准输出管道 `unix_socket` 对象实例（只读）；

##### `process::stderr()`
获取进程的错误输出管道 `unix_socket` 对象实例（只读）；

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
