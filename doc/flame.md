## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。目前，flame 中提供了如下功能：
1. 协程 API；
2. 协程式网络 API；
	1. HTTP 客户端、服务端；
	2. Unix Socket 客户端、服务端；
	3. TCP 客户端、服务端；
	4. UDP 客户端、服务端；
3. 协程式驱动 API：
	1. MySQL 客户端；
	2. Redis 客户端；

**注意**：
* 文档中带 `yield` 前缀的函数为“异步”、“协程式”函数，请在调用时也保持 `yield` 关键字；

## `namespace flame`

最基本的“协程”函数封装，例如生成“协程”，“协程”调度等；

#### `flame\init(string $app_name, array $options)`
可选，设置并初始化框架；

**实例**：
``` php
<?php
flame\init("test_app", [
	"worker" => 4, // 可选，工作进程数，默认为 0
]);

#### `flame\go(callable $g)`
生成一个“协程”；

**示例**：
``` php
<?php
function g1() {
	yield 1;
	yield 2;
}
// 传入 Generator 函数的定义：
flame\go(g1);
flame\go(function() {
	yield 3;
	yield 4;

});
```

**注意**：
* 若传入的 `callable ` 是 `Generator Function` 即包含 yield 表达式的函数，能够支持异步调用；否则仅相当于调用该普通函数；
* 协程**不会立刻启动**（需要等待 `flame\run()` 函数的调度执行），但本函数会立即返回；

#### `flame\config(array $opts)`
配置 `flame` 框架，目前支持以下选项：
``` PHP
<?php
flame\config([
	"worker_count" => 4, // 可选，工作进程数量
]);
```

**注意**：
* 本函数必须在下述 `flame\run()` 函数前执行；否则无法生效；

#### `flame\on_quit(callable $cb)`
添加退出前的回调函数；一般用于在需要终止进程时，进行补充操作、记录日志等，例如 `$server->close()` 等；

**注意**：
* 本函数可重复调用，添加多个回调函数；
* 函数按添加顺序相反的顺序执行（实际为 `stack` 容器）；
* 当进程要结束时还有异步流程没有执行完毕，将会在 10s 后强制终止；

#### `flame\run()`
框架入口，所有协程调度在此处开始执行，直到所有“协程”结束；

**示例**：
``` PHP
<?php
flame\go(function() {
	// yield ...
});
flame\run(); // 实际程序在此循环调度
```

**注意**:
* 此函数调用一般放置在入口执行文件的最后，开始后本函数会阻塞，直到所有协程执行结束；
