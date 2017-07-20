
**flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。目前，flame 中提供了如下功能：
1. 协程 API；
2. 伪异步任务（工作线程）API；
3. 协程式网络 API；
	1. HTTP 客户端、服务端；
	2. Unix Socket 客户端、服务端； 
	3. TCP 客户端、服务端；
	4. UDP 客户端、服务端；
4. 协程式驱动 API：
	1. MySQL 客户端；
	2. Redis 客户端；

### flame

#### flame\go
功能：
	启动一个“协程”；注意协程**不会立刻启动**（等待 `flame\run()` 函数的调度执行），但本函数会立即返回；
原型：
```
	function flame\go(generator g);
```
示例：
```
function g1() {
	yield 1;
	yield 2;
}
// 形式1：传入 Generator 函数的定义：
flame\go(g1);
// 形式2：传入 Generator 函数的调用（Generator 对象）；
flame\go(g1());
```

#### flame\run
功能：
	框架入口，所有协程调度在此处开始执行，直到结束；注意 run() 一般放置在入口执行文件的最后，开始后本函数会阻塞，直到所有协程执行结束；
原型：
```
	function flame\run();
```
示例：
```
function g1() { 
	// ...
}
flame\go(g1);
flame\run();
```