## `namespace flame\os`
封装系统相关的 API，网卡信息, 异步进程启停输出读取等；

#### `array flame\os\interfaces()`
获取当前机器网卡相关信息，返回数组示例如下：
``` PHP
array(2) {
  ["lo"]=>
  array(2) {
    [0]=>
    array(5) {
      ["address"]=>
      string(9) "127.0.0.1"
      ["family"]=>
      string(4) "IPv4"
    }
    [1]=>
    array(6) {
      ["address"]=>
      string(3) "::1"
      ["family"]=>
      string(4) "IPv6"
    }
  }
  ["eth0"]=>
  array(2) {
    [0]=>
    array(5) {
      ["address"]=>
      string(10) "11.22.33.44"
      ["family"]=>
      string(4) "IPv4"
    }
    [1]=>
    array(6) {
      ["address"]=>
      string(25) "fefe:fefe:fefe:fefe:fefe:fefe"
      ["family"]=>
      string(4) "IPv6"
    }
  }
}
```

#### `flame\os\process flame\os\spawn(string $exec[, array $argv[, array $opts])`
启动一个子进程，相关参数如下：
* `$exec` - string - 可执行文件名或路径；
* `$argv` - array | null - 参数；
* `$opts` - array - 其他选项，存在如下选项：
	* `cwd` - string - 工作路径（目录）
	* `env` - map - 环境变量 (默认继承当前进程环境变量, 这里的设置项覆盖默认值)
	* `stdout` - `true` : process::output() | `false`: >/dev/null | `string` - 重定向指定文件;
	* `stderr` - `true` : process::error() | `false`: >/dev/null | `string` - 重定向指定文件;
	* `stdin` - `string` 将指定文本输入给子进程;
函数成功，则返回启动的进程对应的 `process` 对象；否则抛出异常；

**示例**：
``` PHP
<?php
flame\go(function() {
	$proc = flame\os\spawn("ping", ["www.baidu.com"], [
		"env" => ["ENV_KEY_1"=>"ENV_VAL_1"],
		"cwd" => "/tmp",
		"stdout" => true, // 待读取
		"stderr" => "/tmp/error.log", // 重定向管道, 可读取
		"stdin"  => ""
	]);
	// 等待执行完成
	yield $proc->wait();
	// 读取错误输出
	$out = $proc->output(); // 导入文件了, 这里是空的
	$err = $proc->error();
})
```

**注意**：
* 当进程对象销毁时，实际进程将被强制结束 (SIGKILL)；
* 被启动的进程将会“异步”运行 (可以选择进行 `wait()` 等待其执行完毕)；

#### `yield flame\os\exec(string $exec[, array $argv[, array $opts]) -> string`
启动进程，并返回该进程的输出文本数据 (参数 选项与 `spawn()` 完全一致);

**示例**：
``` PHP
<?php
// ....
$output = yield flame\os\exec("ps", ["-ef"]);
echo $output, "\n";
```

### `class flame\os\process`
进程对象;

#### `integer process::$pid`
进程ID；

#### `string process::stdout()`
获取进程的标准输出；

#### `string process::stderr()`
获取进程的错误输出；

#### `void process::kill([$signal = SIGTERM])`
向当前进程发送信号，默认 SIGTERM 信号；

#### `yield process::wait() -> void`
等待当前进程结束；

**注意**：
* 不能在多个协程中对同一进程对象调用 `wait()`；

