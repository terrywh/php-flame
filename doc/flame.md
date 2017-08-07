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

#### `flame\go(callable cb)`
**功能**：
	生成一个“协程”；

**示例**：
``` php
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
* `cb` 必须是 `Generator Function` 即 函数中包含 `yield` 表达式；
* 协程**不会立刻启动**（需要等待 `flame\run()` 函数的调度执行），但本函数会立即返回；

#### `flame\run()`
**功能**：
	框架入口，所有协程调度在此处开始执行，直到所有“协程”结束；


**示例**：
``` php
flame\go(function() {
	// yield ...
});
flame\run();
```

**注意**:
* 此函数调用一般放置在入口执行文件的最后，开始后本函数会阻塞，直到所有协程执行结束；

#### `flame\fork()`
**功能**：
	类似于系统 `fork()` 函数，用于分离出子进程（能够继承文件句柄）；在父进程返回新的子进程 PID，在子进程中返回 0；
